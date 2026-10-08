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
constexpr uint32_t Slot_AccumulationImage = 7;

// FNV-1a over the raw bytes of whatever is added (only types without padding), to tell if what is rendered has changed
class StateHash
{
public:
	template<typename T>
	void add(const T& value)
	{
		const auto* bytes = reinterpret_cast<const unsigned char*>(&value);
		for (size_t i = 0; i < sizeof(T); i++) {
			hash = (hash ^ bytes[i]) * 1099511628211ull;
		}
	}
	uint64_t get() const { return hash; }

private:
	uint64_t hash = 14695981039346656037ull;
};

class GpuPathTracerCS : public ComputeShader
{
public:
	struct PushData {
		uint32_t maxRayDepth;
		uint32_t sampleCount; // samples per pixel accumulated before this dispatch
		uint32_t maxSpp;      // once sampleCount reaches this, nothing is traced anymore
	};

	const DescriptorSet* descriptorSetPtr = nullptr;
	const DescriptorSet* skyDescriptorSetPtr = nullptr;
	const DescriptorSet* bindlessDescriptorSetPtr = nullptr;
	PushData pushData = {1, 0, 1};

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

	// full float precision, so the running average doesn't stall after many samples
	ImageCreator accumulationImageCreator(
		VK_FORMAT_R32G32B32A32_SFLOAT,
		{renderExtent.width, renderExtent.height, 1},
		VK_IMAGE_USAGE_STORAGE_BIT,
		VK_IMAGE_ASPECT_COLOR_BIT,
		"gpuPathTracerAccumulation");
	accumulationImage = new Texture2D(accumulationImageCreator);

	// both stay in GENERAL while the compute pass reads and writes them; the output is only moved to TRANSFER_SRC for the blit
	Vulkan::Instance->immediateSubmit([this](VkCommandBuffer cmdbuf)
	{
		for (Texture2D* image : {outImage, accumulationImage}) {
			vk::insertImageBarrier(cmdbuf, image->resource.image,
								   {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
								   VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
								   VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
								   0,
								   VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
								   VK_IMAGE_LAYOUT_UNDEFINED,
								   VK_IMAGE_LAYOUT_GENERAL);
		}
	});

	sceneTlas.init("GpuPathTracer");
	skyAtmosphereRender.init();

	config = new ConfigAsset("config/gpuPathTracer.ini", true, [this](const ConfigAsset*) {
		sampleCount = 0; // any setting may change the image
	});

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
	layout.addBinding(Slot_AccumulationImage, VK_SHADER_STAGE_COMPUTE_BIT, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);

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
		fd.descriptorSet.pointToStorageImageView(accumulationImage->imageView, Slot_AccumulationImage);
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
	delete accumulationImage;
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
		// Everything the accumulated samples depend on. When any of it changes they are stale, and accumulation starts over.
		StateHash stateHash;
		stateHash.add(viewInfo.ViewMatrix);
		stateHash.add(viewInfo.ProjectionMatrix);
		stateHash.add(viewInfo.BackgroundOption);
		stateHash.add(viewInfo.NumPointLights);
		for (int i = 0; i < viewInfo.NumPointLights; i++) {
			stateHash.add(pointLights[i].position);
			stateHash.add(pointLights[i].color);
		}
		stateHash.add(viewInfo.NumDirectionalLights);
		for (int i = 0; i < viewInfo.NumDirectionalLights; i++) {
			stateHash.add(directionalLights[i].direction);
			stateHash.add(directionalLights[i].color);
		}
		stateHash.add(sky->enabled());
		if (sky->enabled()) stateHash.add(sky->getParameters());
		stateHash.add(meshes.size());
		for (const MeshObject* mo : meshes) {
			stateHash.add(mo->object_to_world());
			stateHash.add(mo->mesh.surface.bindlessMaterialIndex);
			stateHash.add(mo->mesh.gpu_data.geometryRecordIndex);
		}
		if (stateHash.get() != sceneStateHash) {
			sceneStateHash = stateHash.get();
			sampleCount = 0;
		}
	}

	const uint32_t maxSpp = static_cast<uint32_t>(std::max(1, config->lookup<int>("MaxSpp")));

	// once every pixel has all its samples, nothing is traced anymore; the average is only redisplayed
	if (sampleCount < maxSpp) {
		{
			SCOPED_DRAW_EVENT(cmdbuf, "GpuPathTracer rebuild TLAS")
			sceneTlas.build_from_meshes(cmdbuf, frameIndex, meshes, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
		}
		skyAtmosphereRender.update_luts(cmdbuf, sky);
	}

	const VkImageSubresourceRange colorRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
	{
		SCOPED_DRAW_EVENT(cmdbuf, "GpuPathTracer trace")
		ASSERT(BindlessResources::Instance != nullptr)
		auto* pathTracerCS = ComputeShader::getInstance<GpuPathTracerCS>();
		pathTracerCS->descriptorSetPtr = &fd.descriptorSet;
		pathTracerCS->skyDescriptorSetPtr = &skyAtmosphereRender.get_descriptor_set(sky);
		pathTracerCS->bindlessDescriptorSetPtr = &BindlessResources::Instance->descriptorSet();
		pathTracerCS->pushData = {
			static_cast<uint32_t>(std::max(1, config->lookup<int>("MaxRayDepth"))),
			sampleCount,
			maxSpp};
		pathTracerCS->dispatch(
			cmdbuf,
			(renderExtent.width + GPT_GROUPSIZE_X - 1) / GPT_GROUPSIZE_X,
			(renderExtent.height + GPT_GROUPSIZE_Y - 1) / GPT_GROUPSIZE_Y,
			1);
	}

	if (sampleCount < maxSpp) {
		// next frame's dispatch reads and adds to what this one wrote
		vk::insertImageBarrier(cmdbuf, accumulationImage->resource.image, colorRange,
							   VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
							   VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
							   VK_ACCESS_SHADER_WRITE_BIT,
							   VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
							   VK_IMAGE_LAYOUT_GENERAL,
							   VK_IMAGE_LAYOUT_GENERAL);
		sampleCount++;
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
	const uint32_t maxSpp = static_cast<uint32_t>(std::max(1, config->lookup<int>("MaxSpp")));
	auto yellow = ImVec4(1.0f, 0.7f, 0.1f, 1.0f);
	auto green = ImVec4(0.2f, 1.0f, 0.2f, 1.0f);
	ImGui::TextColored(sampleCount < maxSpp ? yellow : green, "samples per pixel: %u / %u", sampleCount, maxSpp);
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
