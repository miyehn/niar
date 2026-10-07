#include "GpuPathTracer.h"
#include "Assets/ConfigAsset.hpp"
#include "Assets/EnvironmentMapAsset.h"
#include "Render/BindlessResources.h"
#include "Render/Materials/ComputeShader.h"
#include "Render/Texture.h"
#include "Render/Vulkan/ImageCreator.h"
#include "Render/Vulkan/VulkanUtils.h"
#include "Scene/MeshObject.h"
#include "Scene/SkyAtmosphere.h"

#include <algorithm>
#include <imgui.h>

#define GPT_GROUPSIZE_X 8
#define GPT_GROUPSIZE_Y 8

namespace
{

constexpr uint32_t Slot_ViewInfo = 0;
constexpr uint32_t Slot_Tlas = 1;
constexpr uint32_t Slot_OutputImage = 2;
constexpr uint32_t Slot_SceneInstanceRecords = 3;
constexpr uint32_t Slot_PointLights = 4;
constexpr uint32_t Slot_DirectionalLights = 5;
constexpr uint32_t Slot_EnvironmentMap = 6;

struct PushData {
	uint32_t maxRayDepth;
};

const ConfigAsset* getGpuPathTracerConfig()
{
	static ConfigAsset* config = new ConfigAsset("config/gpuPathTracer.ini", true);
	return config;
}

class GpuPathTracerCS : public ComputeShader
{
public:
	const DescriptorSet* descriptorSetPtr = nullptr;
	const DescriptorSet* skyDescriptorSetPtr = nullptr;
	const DescriptorSet* bindlessDescriptorSetPtr = nullptr;
	PushData pushData = {1};

	void dispatch(VkCommandBuffer cmdbuf, int groupCountX, int groupCountY, int groupCountZ) override
	{
		ASSERT(cmdbuf != VK_NULL_HANDLE)
		ASSERT(descriptorSetPtr != nullptr)
		ASSERT(skyDescriptorSetPtr != nullptr)
		ASSERT(bindlessDescriptorSetPtr != nullptr)

		auto& pipeline = getPipeline();
		vkCmdBindPipeline(cmdbuf, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.pipeline);
		vkCmdPushConstants(cmdbuf, pipeline.layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(PushData), &pushData);
		descriptorSetPtr->bind(cmdbuf, VK_PIPELINE_BIND_POINT_COMPUTE, DSET_FRAMEGLOBAL, pipeline.layout);
		skyDescriptorSetPtr->bind(cmdbuf, VK_PIPELINE_BIND_POINT_COMPUTE, DSET_INDEPENDENT, pipeline.layout);
		bindlessDescriptorSetPtr->bind(cmdbuf, VK_PIPELINE_BIND_POINT_COMPUTE, DSET_BINDLESS, pipeline.layout);
		vkCmdDispatch(cmdbuf, groupCountX, groupCountY, groupCountZ);
	}

protected:
	void configurePipeline(ComputePipelineBuilder& builder) override
	{
		ASSERT(descriptorSetPtr != nullptr)
		ASSERT(skyDescriptorSetPtr != nullptr)
		ASSERT(bindlessDescriptorSetPtr != nullptr)
		builder.shaderDef = ShaderModuleDef("shaders/gpu_path_tracer.comp", "main", SS_Compute);
		builder.useDescriptorSetLayout(DSET_FRAMEGLOBAL, descriptorSetPtr->getLayout());
		builder.useDescriptorSetLayout(DSET_INDEPENDENT, skyDescriptorSetPtr->getLayout());
		builder.useDescriptorSetLayout(DSET_BINDLESS, bindlessDescriptorSetPtr->getLayout());
		builder.usePushConstantRange({VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(PushData)});
	}
};

}

GpuPathTracer::GpuPathTracer()
{
	const auto& renderExtent = Vulkan::Instance->swapChainExtent;

	ImageCreator imageCreator(
		VK_FORMAT_R8G8B8A8_UNORM,
		{renderExtent.width, renderExtent.height, 1},
		VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
		VK_IMAGE_ASPECT_COLOR_BIT,
		"gpuPathTracerOutput");
	outImage = new Texture2D(imageCreator);
	// the output stays in GENERAL while the compute pass writes it; it's only moved to TRANSFER_SRC for the blit
	Vulkan::Instance->immediateSubmit([this](VkCommandBuffer cmdbuf)
	{
		vk::insertImageBarrier(cmdbuf, outImage->resource.image,
							   {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
							   VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
							   VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
							   0,
							   VK_ACCESS_SHADER_WRITE_BIT,
							   VK_IMAGE_LAYOUT_UNDEFINED,
							   VK_IMAGE_LAYOUT_GENERAL);
	});

	sceneTlas.init("GpuPathTracer");
	skyAtmosphereRender.init();

	const Texture2D* envmap = Config->lookup<int>("LoadEnvironmentMap")
		? Asset::find<EnvironmentMapAsset>(Config->lookup<std::string>("EnvironmentMap"))->texture2D
		: Texture2D::black();

	DescriptorSetLayout layout{};
	layout.addBinding(Slot_ViewInfo, VK_SHADER_STAGE_COMPUTE_BIT, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
	layout.addBinding(Slot_Tlas, VK_SHADER_STAGE_COMPUTE_BIT, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR);
	layout.addBinding(Slot_OutputImage, VK_SHADER_STAGE_COMPUTE_BIT, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
	layout.addBinding(Slot_SceneInstanceRecords, VK_SHADER_STAGE_COMPUTE_BIT, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
	layout.addBinding(Slot_PointLights, VK_SHADER_STAGE_COMPUTE_BIT, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
	layout.addBinding(Slot_DirectionalLights, VK_SHADER_STAGE_COMPUTE_BIT, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
	layout.addBinding(Slot_EnvironmentMap, VK_SHADER_STAGE_COMPUTE_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		auto& fd = gpuFrameData[i];
		fd.viewInfoUbo = VmaBuffer({
			&Vulkan::Instance->memoryAllocator,
			sizeof(glm::ViewInfo),
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VMA_MEMORY_USAGE_CPU_TO_GPU,
			"GpuPathTracer view info UBO"});
		fd.pointLightsUbo = VmaBuffer({
			&Vulkan::Instance->memoryAllocator,
			sizeof(pointLights),
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VMA_MEMORY_USAGE_CPU_TO_GPU,
			"GpuPathTracer point lights UBO"});
		fd.directionalLightsUbo = VmaBuffer({
			&Vulkan::Instance->memoryAllocator,
			sizeof(directionalLights),
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VMA_MEMORY_USAGE_CPU_TO_GPU,
			"GpuPathTracer directional lights UBO"});
		fd.descriptorSet = DescriptorSet(layout);
		fd.descriptorSet.pointToBuffer(fd.viewInfoUbo, Slot_ViewInfo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		fd.descriptorSet.pointToAccelerationStructure(sceneTlas.get(), Slot_Tlas);
		fd.descriptorSet.pointToStorageImageView(outImage->imageView, Slot_OutputImage);
		fd.descriptorSet.pointToBuffer(sceneTlas.getSceneInstanceRecordBuffer(i), Slot_SceneInstanceRecords, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
		fd.descriptorSet.pointToBuffer(fd.pointLightsUbo, Slot_PointLights, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		fd.descriptorSet.pointToBuffer(fd.directionalLightsUbo, Slot_DirectionalLights, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		fd.descriptorSet.pointToImageView(envmap->imageView, Slot_EnvironmentMap);
	}
}

GpuPathTracer::~GpuPathTracer()
{
	sceneTlas.release();
	skyAtmosphereRender.release();
	for (auto& fd : gpuFrameData) {
		fd.viewInfoUbo.release();
		fd.pointLightsUbo.release();
		fd.directionalLightsUbo.release();
	}
	delete outImage;
}

void GpuPathTracer::render(VkCommandBuffer cmdbuf)
{
	if (!camera) return;

	const uint32_t frameIndex = Vulkan::Instance->getCurrentFrameIndex();
	auto& fd = gpuFrameData[frameIndex];

	std::vector<MeshObject*> meshes;
	drawable->foreach_descendent_bfs([&meshes](SceneObject* obj) {
		if (auto* mo = dynamic_cast<MeshObject*>(obj)) meshes.push_back(mo);
	}, [](SceneObject* obj) { return obj->enabled(); });

	SkyAtmosphere* sky = SkyAtmosphere::getInstance();

	const auto& renderExtent = Vulkan::Instance->swapChainExtent;
	glm::ViewInfo viewInfo = getCameraViewInfo(glm::vec2(renderExtent.width, renderExtent.height));
	viewInfo.Exposure = cfgExposure;
	viewInfo.BackgroundOption = getBackgroundOption(sky);

	gatherLights(drawable, pointLights, directionalLights, viewInfo);

	fd.viewInfoUbo.writeData(&viewInfo, sizeof(viewInfo));
	fd.pointLightsUbo.writeData(pointLights, viewInfo.NumPointLights * sizeof(glm::PointLightInfo));
	fd.directionalLightsUbo.writeData(directionalLights, viewInfo.NumDirectionalLights * sizeof(glm::DirectionalLightInfo));

	{
		SCOPED_DRAW_EVENT(cmdbuf, "GpuPathTracer rebuild TLAS")
		sceneTlas.build_from_meshes(cmdbuf, frameIndex, meshes, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	}

	skyAtmosphereRender.update_luts(cmdbuf, sky);

	const VkImageSubresourceRange colorRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
	{
		SCOPED_DRAW_EVENT(cmdbuf, "GpuPathTracer trace")
		ASSERT(BindlessResources::Instance != nullptr)
		auto* pathTracerCS = ComputeShader::getInstance<GpuPathTracerCS>();
		pathTracerCS->descriptorSetPtr = &fd.descriptorSet;
		pathTracerCS->skyDescriptorSetPtr = &skyAtmosphereRender.get_descriptor_set(sky);
		pathTracerCS->bindlessDescriptorSetPtr = &BindlessResources::Instance->descriptorSet();
		pathTracerCS->pushData.maxRayDepth = static_cast<uint32_t>(std::max(1, getGpuPathTracerConfig()->lookup<int>("MaxRayDepth")));
		pathTracerCS->dispatch(
			cmdbuf,
			(renderExtent.width + GPT_GROUPSIZE_X - 1) / GPT_GROUPSIZE_X,
			(renderExtent.height + GPT_GROUPSIZE_Y - 1) / GPT_GROUPSIZE_Y,
			1);
	}

	vk::insertImageBarrier(cmdbuf, outImage->resource.image, colorRange,
						   VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
						   VK_PIPELINE_STAGE_TRANSFER_BIT,
						   VK_ACCESS_SHADER_WRITE_BIT,
						   VK_ACCESS_TRANSFER_READ_BIT,
						   VK_IMAGE_LAYOUT_GENERAL,
						   VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

	vk::blitToScreen(
		cmdbuf,
		outImage->resource.image,
		{0, 0, 0},
		{(int32_t)renderExtent.width, (int32_t)renderExtent.height, 1});

	// the blit only reads the image, so just order it before next frame's compute write
	vk::insertImageBarrier(cmdbuf, outImage->resource.image, colorRange,
						   VK_PIPELINE_STAGE_TRANSFER_BIT,
						   VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
						   0,
						   VK_ACCESS_SHADER_WRITE_BIT,
						   VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
						   VK_IMAGE_LAYOUT_GENERAL);
}

void GpuPathTracer::draw_config_ui()
{
	ImGui::SliderFloat("##exposure", &cfgExposure, -25, 25, "exposure comp: %.3f");
}

GpuPathTracer* GpuPathTracer::get()
{
	static GpuPathTracer* renderer = nullptr;
	if (renderer == nullptr)
	{
		renderer = new GpuPathTracer();
	}
	return renderer;
}
