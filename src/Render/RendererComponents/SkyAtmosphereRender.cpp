#include "SkyAtmosphereRender.h"

#include "Render/Materials/ComputeShader.h"
#include "Render/Texture.h"
#include "Render/Vulkan/ImageCreator.h"
#include "Render/Vulkan/SamplerCache.h"
#include "Render/Vulkan/VulkanUtils.h"
#include "../../Scene/SkyAtmosphere.h"

#define CS_GROUPSIZE_X 8
#define CS_GROUPSIZE_Y 8

// checklist: https://community.khronos.org/t/drawing-to-image-from-compute-shader-example/7116/2
class TransmittanceLutCS : public ComputeShader {
public:
	const DescriptorSet* descriptorSetPtr = nullptr;
	void dispatch(VkCommandBuffer cmdbuf, int groupCountX, int groupCountY, int groupCountZ) override {
		ASSERT(cmdbuf != VK_NULL_HANDLE)
		ASSERT(descriptorSetPtr != nullptr)

		auto& pipeline = getPipeline();
		vkCmdBindPipeline(cmdbuf, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.pipeline);
		descriptorSetPtr->bind(cmdbuf, VK_PIPELINE_BIND_POINT_COMPUTE, DSET_INDEPENDENT, pipeline.layout);
		vkCmdDispatch(cmdbuf, groupCountX, groupCountY, groupCountZ);
	}
protected:
	void configurePipeline(ComputePipelineBuilder& builder) override
	{
		ASSERT(descriptorSetPtr != nullptr)
		builder.shaderDef = ShaderModuleDef("shaders/sky_transmittance_lut.comp", "main", SS_Compute);
		builder.useDescriptorSetLayout(DSET_INDEPENDENT, descriptorSetPtr->getLayout());
	}
	friend class ComputeShader;
};

class SkyViewLutCS : public ComputeShader {
public:
	const DescriptorSet* descriptorSetPtr = nullptr;
	// dispatch fn
	void dispatch(VkCommandBuffer cmdbuf, int groupCountX, int groupCountY, int groupCountZ) override {
		ASSERT(cmdbuf != VK_NULL_HANDLE)
		ASSERT(descriptorSetPtr != nullptr)

		auto& pipeline = getPipeline();
		vkCmdBindPipeline(cmdbuf, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.pipeline);
		descriptorSetPtr->bind(cmdbuf, VK_PIPELINE_BIND_POINT_COMPUTE, DSET_INDEPENDENT, pipeline.layout);
		vkCmdDispatch(cmdbuf, groupCountX, groupCountY, groupCountZ);
	}

protected:
	void configurePipeline(ComputePipelineBuilder& builder) override
	{
		ASSERT(descriptorSetPtr != nullptr)
		builder.shaderDef = ShaderModuleDef("shaders/sky_view_lut.comp", "main", SS_Compute);
		builder.useDescriptorSetLayout(DSET_INDEPENDENT, descriptorSetPtr->getLayout());
	}
	friend class ComputeShader;
};


void SkyAtmosphereRender::init()
{
	SkyAtmosphere* sky = SkyAtmosphere::getInstance();

	const auto transmittanceDims = sky->getTransmittanceLutDimensions();
	ImageCreator transmittanceLutCreator(
		VK_FORMAT_R16G16B16A16_SFLOAT,
		{transmittanceDims.x, transmittanceDims.y, 1},
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
		VK_IMAGE_ASPECT_COLOR_BIT,
		"Transmittance LUT");
	transmittanceLut = new Texture2D(transmittanceLutCreator);

	const auto skyViewDims = sky->getSkyViewLutDimensions();
	ImageCreator skyViewLutCreator(
		VK_FORMAT_R16G16B16A16_SFLOAT,
		{skyViewDims.x, skyViewDims.y, 1},
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
		VK_IMAGE_ASPECT_COLOR_BIT,
		"Sky View LUT");
	skyViewLut = new Texture2D(skyViewLutCreator);

	// initialize luts into SHADER_READ_ONLY_OPTIMAL
	Vulkan::Instance->immediateSubmit([this](VkCommandBuffer cmdbuf)
	{
		const VkImageSubresourceRange colorRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
		vk::insertImageBarrier(
			cmdbuf,
			transmittanceLut->resource.image,
			colorRange,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			0,
			VK_ACCESS_SHADER_READ_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		vk::insertImageBarrier(
			cmdbuf,
			skyViewLut->resource.image,
			colorRange,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			0,
			VK_ACCESS_SHADER_READ_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	});

	DescriptorSetLayout skySetLayout{};
	skySetLayout.addBinding(SkyAtmosphere::Slot_Parameters, VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
	skySetLayout.addBinding(SkyAtmosphere::Slot_TransmittanceLutRW, VK_SHADER_STAGE_COMPUTE_BIT, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
	skySetLayout.addBinding(SkyAtmosphere::Slot_SkyViewLutRW, VK_SHADER_STAGE_COMPUTE_BIT, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
	skySetLayout.addBinding(SkyAtmosphere::Slot_TransmittanceLutR, VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	skySetLayout.addBinding(SkyAtmosphere::Slot_SkyViewLutR, VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

	auto samplerInfo = SamplerCache::defaultInfo();
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		auto& fd = gpuFrameData[i];
		fd.parametersBuffer = VmaBuffer({
			&Vulkan::Instance->memoryAllocator,
			sizeof(SkyAtmosphere::Parameters),
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VMA_MEMORY_USAGE_CPU_TO_GPU,
			"Sky atmosphere parameters buffer"
		});

		fd.descriptorSet = DescriptorSet(skySetLayout);
		fd.descriptorSet.pointToBuffer(fd.parametersBuffer, SkyAtmosphere::Slot_Parameters, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		fd.descriptorSet.pointToRWImageView(transmittanceLut->imageView, SkyAtmosphere::Slot_TransmittanceLutRW);
		fd.descriptorSet.pointToRWImageView(skyViewLut->imageView, SkyAtmosphere::Slot_SkyViewLutRW);
		fd.descriptorSet.pointToImageView(transmittanceLut->imageView, SkyAtmosphere::Slot_TransmittanceLutR, &samplerInfo);
		fd.descriptorSet.pointToImageView(skyViewLut->imageView, SkyAtmosphere::Slot_SkyViewLutR);

		fd.dummyDescriptorSet = DescriptorSet(skySetLayout);
		fd.dummyDescriptorSet.pointToBuffer(fd.parametersBuffer, SkyAtmosphere::Slot_Parameters, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		fd.dummyDescriptorSet.pointToImageView(Texture::get<Texture2D>("_black")->imageView, SkyAtmosphere::Slot_TransmittanceLutR, &samplerInfo);
		fd.dummyDescriptorSet.pointToImageView(Texture::get<Texture2D>("_black")->imageView, SkyAtmosphere::Slot_SkyViewLutR, &samplerInfo);
	}
}

void SkyAtmosphereRender::release()
{
	for (auto& fd : gpuFrameData) {
		fd.parametersBuffer.release();
	}

	delete transmittanceLut;
	transmittanceLut = nullptr;

	delete skyViewLut;
	skyViewLut = nullptr;
}

void SkyAtmosphereRender::update_luts(VkCommandBuffer cmdbuf, SkyAtmosphere* sky)
{
	if (!sky || !sky->enabled()) return;

	auto& fd = gpuFrameData[Vulkan::Instance->getCurrentFrameIndex()];
	auto parameters = sky->getParameters();
	fd.parametersBuffer.writeData(&parameters, sizeof(parameters));

	const VkImageSubresourceRange colorRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

	{
		SCOPED_DRAW_EVENT(cmdbuf, "Dispatch TransmittanceLutCS")
		auto transmittanceCS = ComputeShader::getInstance<TransmittanceLutCS>();
		transmittanceCS->descriptorSetPtr = &fd.descriptorSet;
		vk::insertImageBarrier(
			cmdbuf,
			transmittanceLut->resource.image,
			colorRange,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			VK_ACCESS_SHADER_READ_BIT,
			VK_ACCESS_SHADER_WRITE_BIT,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_IMAGE_LAYOUT_GENERAL);
		transmittanceCS->dispatch(
			cmdbuf,
			(transmittanceLut->getWidth() + CS_GROUPSIZE_X - 1) / CS_GROUPSIZE_X,
			(transmittanceLut->getHeight() + CS_GROUPSIZE_X - 1) / CS_GROUPSIZE_Y,
			1);
		vk::insertImageBarrier(
			cmdbuf,
			transmittanceLut->resource.image,
			colorRange,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			VK_ACCESS_SHADER_WRITE_BIT,
			VK_ACCESS_SHADER_READ_BIT,
			VK_IMAGE_LAYOUT_GENERAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}

	{
		SCOPED_DRAW_EVENT(cmdbuf, "Dispatch SkyViewLutCS")
		auto skyViewCS = ComputeShader::getInstance<SkyViewLutCS>();
		skyViewCS->descriptorSetPtr = &fd.descriptorSet;
		vk::insertImageBarrier(
			cmdbuf,
			skyViewLut->resource.image,
			colorRange,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			VK_ACCESS_SHADER_READ_BIT,
			VK_ACCESS_SHADER_WRITE_BIT,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_IMAGE_LAYOUT_GENERAL);
		skyViewCS->dispatch(
			cmdbuf,
			(skyViewLut->getWidth() + CS_GROUPSIZE_X - 1) / CS_GROUPSIZE_X,
			(skyViewLut->getHeight() + CS_GROUPSIZE_X - 1) / CS_GROUPSIZE_Y,
			1);
		vk::insertImageBarrier(
			cmdbuf,
			skyViewLut->resource.image,
			colorRange,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			VK_ACCESS_SHADER_WRITE_BIT,
			VK_ACCESS_SHADER_READ_BIT,
			VK_IMAGE_LAYOUT_GENERAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}
}

DescriptorSet& SkyAtmosphereRender::get_descriptor_set(const SkyAtmosphere* sky)
{
	auto& fd = gpuFrameData[Vulkan::Instance->getCurrentFrameIndex()];
	return (sky && sky->enabled()) ? fd.descriptorSet : fd.dummyDescriptorSet;
}
