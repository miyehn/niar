#include "BindlessResources.h"

#include "Render/Materials/ComputeShader.h"
#include "Render/Vulkan/SamplerCache.h"
#include "Render/Vulkan/Vulkan.hpp"
#include "Utils/myn/Log.h"

#include <algorithm>
#include <array>

BindlessResources* BindlessResources::Instance = nullptr;

namespace
{

constexpr uint32_t BindlessTextureBinding = 0;
constexpr uint32_t MaterialBufferBinding = 1;

VkDescriptorImageInfo textureDescriptor(
	VkImageView imageView,
	const VkSamplerCreateInfo& samplerInfo)
{
	return {
		.sampler = SamplerCache::get(samplerInfo),
		.imageView = imageView,
		.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
	};
}

#ifdef DEBUG
class BindlessSelfTestCS : public ComputeShader
{
public:
	const DescriptorSet* testDescriptorSet = nullptr;
	const DescriptorSet* bindlessDescriptorSet = nullptr;

	void dispatch(
		VkCommandBuffer cmdbuf,
		int groupCountX,
		int groupCountY,
		int groupCountZ) override
	{
		ASSERT(cmdbuf != VK_NULL_HANDLE)
		ASSERT(testDescriptorSet != nullptr)
		ASSERT(bindlessDescriptorSet != nullptr)

		const auto& pipeline = getPipeline();
		vkCmdBindPipeline(
			cmdbuf,
			VK_PIPELINE_BIND_POINT_COMPUTE,
			pipeline.pipeline);
		testDescriptorSet->bind(
			cmdbuf,
			VK_PIPELINE_BIND_POINT_COMPUTE,
			DSET_FRAMEGLOBAL,
			pipeline.layout);
		bindlessDescriptorSet->bind(
			cmdbuf,
			VK_PIPELINE_BIND_POINT_COMPUTE,
			DSET_BINDLESS,
			pipeline.layout);
		vkCmdDispatch(cmdbuf, groupCountX, groupCountY, groupCountZ);
	}

protected:
	void configurePipeline(ComputePipelineBuilder& builder) override
	{
		ASSERT(testDescriptorSet != nullptr)
		ASSERT(bindlessDescriptorSet != nullptr)

		builder.shaderDef =
			ShaderModuleDef("shaders/bindless_self_test.comp", "main", SS_Compute);
		builder.useDescriptorSetLayout(
			DSET_FRAMEGLOBAL,
			testDescriptorSet->getLayout());
		builder.useDescriptorSetLayout(
			DSET_BINDLESS,
			bindlessDescriptorSet->getLayout());
	}
};
#endif

}

void BindlessResources::init()
{
	ASSERT(Instance == nullptr)
	ASSERT(Vulkan::Instance != nullptr)

	Instance = this;

	{ // create the dedicated pool
		const std::array<VkDescriptorPoolSize, 2> poolSizes = {{
			{
				.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				.descriptorCount = MAX_BINDLESS_TEXTURES_2D,
			},
			{
				.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				.descriptorCount = 1,
			},
		}};
		const VkDescriptorPoolCreateInfo poolInfo = {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
			.maxSets = 1,
			.poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
			.pPoolSizes = poolSizes.data(),
		};
		EXPECT(vkCreateDescriptorPool(
			Vulkan::Instance->device,
			&poolInfo,
			nullptr,
			&descriptorPool), VK_SUCCESS)
	}

	{ // allocate the bindless set from the dedicated pool
		constexpr VkShaderStageFlags bindlessStages =
			VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
		bindlessSetLayout.addBinding(
			BindlessTextureBinding,
			bindlessStages,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			MAX_BINDLESS_TEXTURES_2D);
		bindlessSetLayout.addBinding(
			MaterialBufferBinding,
			bindlessStages,
			VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
		bindlessDescriptorSet = DescriptorSet(bindlessSetLayout, descriptorPool);
	}

	{ // create placeholder material buffer and put to slot 1
		placeholderMaterialBuffer = VmaBuffer({
			.allocator = &Vulkan::Instance->memoryAllocator,
			.strideSize = 16,
			.bufferUsage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			.memoryUsage = VMA_MEMORY_USAGE_CPU_TO_GPU,
			.debugName = "Bindless placeholder material buffer",
		});
		std::array<uint8_t, 16> zeroPlaceholder{};
		placeholderMaterialBuffer.writeData(
			zeroPlaceholder.data(),
			zeroPlaceholder.size());
		bindlessDescriptorSet.pointToBuffer(
			placeholderMaterialBuffer,
			MaterialBufferBinding,
			VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
		ASSERT(placeholderMaterialBuffer.buffer != VK_NULL_HANDLE)
	}

	ASSERT(freeTexture2DSlots.empty())
	freeTexture2DSlots.reserve(MAX_BINDLESS_TEXTURES_2D);
	for (uint32_t i = 0; i < MAX_BINDLESS_TEXTURES_2D; ++i)
	{
		texture2DSlots[i] = {};
		freeTexture2DSlots.push_back(MAX_BINDLESS_TEXTURES_2D - 1 - i);
	}
}

void BindlessResources::release()
{
	ASSERT(Instance == this)

	const uint32_t occupiedCount = static_cast<uint32_t>(std::count_if(
		texture2DSlots.begin(),
		texture2DSlots.end(),
		[](const Texture2DSlot& slot) { return slot.occupied; }));
	ASSERT_M(
		occupiedCount == 0,
		"BindlessResources still has %u texture registration(s)",
		occupiedCount)

	vkDestroyDescriptorPool(Vulkan::Instance->device, descriptorPool, nullptr);
	descriptorPool = VK_NULL_HANDLE;
	bindlessDescriptorSet = {};
	bindlessSetLayout = {};
	fillerTexture2DDescriptor = {};

	placeholderMaterialBuffer.release();

	for (auto& slot : texture2DSlots) slot = {};
	freeTexture2DSlots.clear();
	Instance = nullptr;
}

BindlessTexture2DHandle BindlessResources::addTexture2D(
	VkImageView imageView,
	const VkSamplerCreateInfo& samplerInfo)
{
	ASSERT(Instance == this)
	ASSERT(imageView != VK_NULL_HANDLE)

	if (freeTexture2DSlots.empty())
	{
		ERR("Bindless 2D texture table is exhausted")
		return {};
	}

	// take a slot from free list
	const uint32_t slotIndex = freeTexture2DSlots.back();
	freeTexture2DSlots.pop_back();
	// (this is the slot it's just taken from free list must be free)
	auto& slot = texture2DSlots[slotIndex];
	ASSERT(!slot.occupied)

	// create a descriptor for input image and update bindless
	const VkDescriptorImageInfo descriptor =
		textureDescriptor(imageView, samplerInfo);
	bindlessDescriptorSet.pointToImageViews(
		BindlessTextureBinding,
		slotIndex,
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		{&descriptor, 1});

	slot.occupied = true;
	return {
		.index = slotIndex,
		.generation = slot.generation,
	};
}

void BindlessResources::removeTexture2D(BindlessTexture2DHandle handle)
{
	const uint32_t slotIndex = validate(handle);
	ASSERT(slotIndex != INVALID_BINDLESS_INDEX)

	Vulkan::Instance->waitDeviceIdle();
	bindlessDescriptorSet.pointToImageViews(
		BindlessTextureBinding,
		slotIndex,
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		{&fillerTexture2DDescriptor, 1});

	auto& slot = texture2DSlots[slotIndex];
	slot.occupied = false;
	++slot.generation;
	freeTexture2DSlots.push_back(slotIndex);
}

void BindlessResources::setTexture2DFiller(
	VkImageView imageView,
	const VkSamplerCreateInfo& samplerInfo)
{
	ASSERT(Instance == this)
	ASSERT(imageView != VK_NULL_HANDLE)

	fillerTexture2DDescriptor = textureDescriptor(imageView, samplerInfo);
	for (uint32_t i = 0; i < MAX_BINDLESS_TEXTURES_2D; ++i)
	{
		if (texture2DSlots[i].occupied) continue;
		bindlessDescriptorSet.pointToImageViews(
			BindlessTextureBinding,
			i,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			{&fillerTexture2DDescriptor, 1});
	}
}

#ifdef DEBUG
// note [myn]: this function is not reviewed
void BindlessResources::runDebugSelfTest(
	BindlessTexture2DHandle whiteHandle,
	BindlessTexture2DHandle blackHandle,
	VkImageView blackImageView,
	const VkSamplerCreateInfo& samplerInfo)
{
	const uint32_t whiteIndex = validate(whiteHandle);
	const uint32_t blackIndex = validate(blackHandle);
	ASSERT(whiteIndex != INVALID_BINDLESS_INDEX)
	ASSERT(blackIndex != INVALID_BINDLESS_INDEX)

	const BindlessTexture2DHandle temporaryHandle =
		addTexture2D(blackImageView, samplerInfo);
	const uint32_t temporaryIndex = temporaryHandle.index;
	const uint32_t temporaryGeneration = temporaryHandle.generation;
	removeTexture2D(temporaryHandle);
	ASSERT(!texture2DSlots[temporaryIndex].occupied)
	ASSERT(texture2DSlots[temporaryIndex].generation == temporaryGeneration + 1)

	std::array textureIndices = {
		whiteIndex,
		temporaryIndex,
	};

	VmaBuffer inputBuffer({
		.allocator = &Vulkan::Instance->memoryAllocator,
		.strideSize = sizeof(textureIndices),
		.bufferUsage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		.memoryUsage = VMA_MEMORY_USAGE_CPU_TO_GPU,
		.debugName = "Bindless self-test input",
	});
	inputBuffer.writeData(textureIndices.data(), sizeof(textureIndices));

	VmaBuffer outputBuffer({
		.allocator = &Vulkan::Instance->memoryAllocator,
		.strideSize = sizeof(float) * 8,
		.bufferUsage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		.memoryUsage = VMA_MEMORY_USAGE_GPU_TO_CPU,
		.debugName = "Bindless self-test output",
	});

	DescriptorSetLayout testSetLayout;
	testSetLayout.addBinding(
		0,
		VK_SHADER_STAGE_COMPUTE_BIT,
		VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
	testSetLayout.addBinding(
		1,
		VK_SHADER_STAGE_COMPUTE_BIT,
		VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
	DescriptorSet testDescriptorSet(testSetLayout);
	testDescriptorSet.pointToBuffer(
		inputBuffer,
		0,
		VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
	testDescriptorSet.pointToBuffer(
		outputBuffer,
		1,
		VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

	auto* selfTest = ComputeShader::getInstance<BindlessSelfTestCS>();
	selfTest->testDescriptorSet = &testDescriptorSet;
	selfTest->bindlessDescriptorSet = &bindlessDescriptorSet;
	Vulkan::Instance->immediateSubmit([selfTest](VkCommandBuffer cmdbuf)
	{
		selfTest->dispatch(cmdbuf, 1, 1, 1);

		const VkMemoryBarrier computeToHostBarrier = {
			.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
			.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
			.dstAccessMask = VK_ACCESS_HOST_READ_BIT,
		};
		vkCmdPipelineBarrier(
			cmdbuf,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			VK_PIPELINE_STAGE_HOST_BIT,
			0,
			1,
			&computeToHostBarrier,
			0,
			nullptr,
			0,
			nullptr);
	});

	std::array<float, 8> sampledColors{};
	outputBuffer.readData(sampledColors.data(), sizeof(sampledColors));

	const auto approximately = [](float actual, float expected)
	{
		return std::abs(actual - expected) <= 0.01f;
	};
	for (uint32_t channel = 0; channel < 4; ++channel)
	{
		ASSERT_M(
			approximately(sampledColors[channel], 1.0f),
			"Bindless self-test expected white channel %u, got %f",
			channel,
			sampledColors[channel])
		ASSERT_M(
			approximately(sampledColors[4 + channel], 0.0f),
			"Bindless self-test expected black channel %u, got %f",
			channel,
			sampledColors[4 + channel])
	}

	const BindlessTexture2DHandle reusedHandle =
		addTexture2D(blackImageView, samplerInfo);
	ASSERT(reusedHandle.index == temporaryIndex)
	ASSERT(reusedHandle.generation == temporaryGeneration + 1)
	ASSERT(temporaryHandle.generation != reusedHandle.generation)
	removeTexture2D(reusedHandle);

	outputBuffer.release();
	inputBuffer.release();
	LOG("Bindless texture self-test passed")
}
#endif

uint32_t BindlessResources::validate(BindlessTexture2DHandle handle) const
{
	if (handle.index == INVALID_BINDLESS_INDEX)
	{
		ERR("Invalid bindless 2D texture handle")
		return INVALID_BINDLESS_INDEX;
	}
	if (handle.index >= MAX_BINDLESS_TEXTURES_2D)
	{
		ERR("Bindless 2D texture handle index %u is out of range", handle.index)
		return INVALID_BINDLESS_INDEX;
	}

	const auto& slot = texture2DSlots[handle.index];
	if (!slot.occupied)
	{
		ERR("Bindless 2D texture handle %u:%u refers to a free slot", handle.index, handle.generation)
		return INVALID_BINDLESS_INDEX;
	}
	if (slot.generation != handle.generation)
	{
		ERR(
			"Stale bindless 2D texture handle %u:%u; current generation is %u",
			handle.index,
			handle.generation,
			slot.generation)
		return INVALID_BINDLESS_INDEX;
	}
	return handle.index;
}
