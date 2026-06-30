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
constexpr uint32_t GeometryRecordBufferBinding = 2;

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

#if TMP_BINDLESS_DEBUG
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
				.descriptorCount = 2,
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
		bindlessSetLayout.addBinding(
			GeometryRecordBufferBinding,
			bindlessStages,
			VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
		bindlessDescriptorSet = DescriptorSet(bindlessSetLayout, descriptorPool);
	}

	// put material table to slot 1
	uploadMaterialTable();
	// geometry record table to slot 2
	uploadGeometryRecordTable();

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
	{ // validation: bindless resources are all unregistered
		const uint32_t occupiedCount = static_cast<uint32_t>(std::count_if(
			texture2DSlots.begin(),
			texture2DSlots.end(),
			[](const Texture2DSlot& slot) { return slot.occupied; }));
		ASSERT_M(
			occupiedCount == 0,
			"BindlessResources still has %u texture registration(s)",
			occupiedCount)
		const uint32_t materialCount = static_cast<uint32_t>(std::count_if(
			materialSlotOccupied.begin(),
			materialSlotOccupied.end(),
			[](uint8_t occupied) { return occupied != 0; }));
		ASSERT_M(
			materialCount == 0,
			"BindlessResources still has %u material registration(s)",
			materialCount)
		const uint32_t geometryRecordCount = static_cast<uint32_t>(std::count_if(
			geometryRecordSlotOccupied.begin(),
			geometryRecordSlotOccupied.end(),
			[](uint8_t occupied) { return occupied != 0; }));
		ASSERT_M(
			geometryRecordCount == 0,
			"BindlessResources still has %u geometry record registration(s)",
			geometryRecordCount)
	}

	vkDestroyDescriptorPool(Vulkan::Instance->device, descriptorPool, nullptr);
	descriptorPool = VK_NULL_HANDLE;
	bindlessDescriptorSet = {};
	bindlessSetLayout = {};
	fillerTexture2DDescriptor = {};

	materialTableBuffer.release();
	geometryRecordTableBuffer.release();

	for (auto& slot : texture2DSlots) slot = {};
	freeTexture2DSlots.clear();

	materialRecords.clear();
	materialSlotOccupied.clear();
	freeMaterialSlots.clear();

	geometryRecords.clear();
	geometryRecordSlotOccupied.clear();
	freeGeometryRecordSlots.clear();

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

uint32_t BindlessResources::addMaterial(const glm::GpuMaterial& material)
{
	ASSERT(Instance == this)

	uint32_t materialIndex;
	if (freeMaterialSlots.empty())
	{
		// no empty slots: push to the end of the list
		materialIndex = static_cast<uint32_t>(materialRecords.size());
		materialRecords.push_back(material);
		materialSlotOccupied.push_back(1);
	}
	else
	{
		// added to an existing empty slot
		materialIndex = freeMaterialSlots.back();
		freeMaterialSlots.pop_back();
		ASSERT(materialIndex < materialRecords.size())
		ASSERT(materialSlotOccupied[materialIndex] == 0)
		materialRecords[materialIndex] = material;
		materialSlotOccupied[materialIndex] = 1;
	}

	uploadMaterialTable();
	return materialIndex;
}

void BindlessResources::updateMaterial(
	uint32_t index,
	const glm::GpuMaterial& material)
{
	ASSERT(Instance == this)
	ASSERT(index < materialRecords.size())
	ASSERT(materialSlotOccupied[index] != 0)

	materialRecords[index] = material;
	uploadMaterialTable();
}

void BindlessResources::removeMaterial(uint32_t index)
{
	ASSERT(Instance == this)
	ASSERT(index < materialRecords.size())
	ASSERT(materialSlotOccupied[index] != 0)

	Vulkan::Instance->waitDeviceIdle();
	materialRecords[index] = {};
	materialSlotOccupied[index] = 0;
	freeMaterialSlots.push_back(index);
	uploadMaterialTable();
}

void BindlessResources::clearMaterials()
{
	ASSERT(Instance == this)

	Vulkan::Instance->waitDeviceIdle();
	materialRecords.clear();
	materialSlotOccupied.clear();
	freeMaterialSlots.clear();
	uploadMaterialTable();
}

uint32_t BindlessResources::addGeometryRecord(const glm::GpuGeometryRecord& geometryRecord)
{
	ASSERT(Instance == this)

	uint32_t geometryRecordIndex;
	if (freeGeometryRecordSlots.empty())
	{
		geometryRecordIndex = static_cast<uint32_t>(geometryRecords.size());
		geometryRecords.push_back(geometryRecord);
		geometryRecordSlotOccupied.push_back(1);
	}
	else
	{
		geometryRecordIndex = freeGeometryRecordSlots.back();
		freeGeometryRecordSlots.pop_back();
		ASSERT(geometryRecordIndex < geometryRecords.size())
		ASSERT(geometryRecordSlotOccupied[geometryRecordIndex] == 0)
		geometryRecords[geometryRecordIndex] = geometryRecord;
		geometryRecordSlotOccupied[geometryRecordIndex] = 1;
	}

	uploadGeometryRecordTable();
	return geometryRecordIndex;
}

void BindlessResources::updateGeometryRecord(
	uint32_t index,
	const glm::GpuGeometryRecord& geometryRecord)
{
	ASSERT(Instance == this)
	ASSERT(index < geometryRecords.size())
	ASSERT(geometryRecordSlotOccupied[index] != 0)

	geometryRecords[index] = geometryRecord;
	uploadGeometryRecordTable();
}

void BindlessResources::removeGeometryRecord(uint32_t index)
{
	ASSERT(Instance == this)
	ASSERT(index < geometryRecords.size())
	ASSERT(geometryRecordSlotOccupied[index] != 0)

	Vulkan::Instance->waitDeviceIdle();
	geometryRecords[index] = {};
	geometryRecordSlotOccupied[index] = 0;
	freeGeometryRecordSlots.push_back(index);
	uploadGeometryRecordTable();
}

void BindlessResources::clearGeometryRecords()
{
	ASSERT(Instance == this)

	Vulkan::Instance->waitDeviceIdle();
	geometryRecords.clear();
	geometryRecordSlotOccupied.clear();
	freeGeometryRecordSlots.clear();
	uploadGeometryRecordTable();
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

#if TMP_BINDLESS_DEBUG
uint32_t BindlessResources::occupiedTexture2DSlotCount() const
{
	return static_cast<uint32_t>(std::count_if(
		texture2DSlots.begin(),
		texture2DSlots.end(),
		[](const Texture2DSlot& slot) { return slot.occupied; }));
}

uint32_t BindlessResources::activeMaterialCount() const
{
	return static_cast<uint32_t>(std::count_if(
		materialSlotOccupied.begin(),
		materialSlotOccupied.end(),
		[](uint8_t occupied) { return occupied != 0; }));
}

uint32_t BindlessResources::activeGeometryRecordCount() const
{
	return static_cast<uint32_t>(std::count_if(
		geometryRecordSlotOccupied.begin(),
		geometryRecordSlotOccupied.end(),
		[](uint8_t occupied) { return occupied != 0; }));
}

void BindlessResources::assertMaterialIndexOccupied(uint32_t index) const
{
	ASSERT(index != INVALID_BINDLESS_INDEX)
	ASSERT_M(
		index < materialSlotOccupied.size(),
		"Bindless material index %u is out of range",
		index)
	ASSERT_M(
		materialSlotOccupied[index] != 0,
		"Bindless material index %u refers to a free slot",
		index)
}

void BindlessResources::assertGeometryRecordIndexOccupied(uint32_t index) const
{
	ASSERT(index != INVALID_SCENE_GEOMETRY_INDEX)
	ASSERT_M(
		index < geometryRecordSlotOccupied.size(),
		"Bindless geometry record index %u is out of range",
		index)
	ASSERT_M(
		geometryRecordSlotOccupied[index] != 0,
		"Bindless geometry record index %u refers to a free slot",
		index)
}

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
	const glm::GpuMaterial debugMaterial = {
		.baseColorFactor = glm::vec4(0.25f, 0.5f, 0.75f, 1.0f),
		.emissiveFactorAndClipThreshold = glm::vec4(0.1f, 0.2f, 0.3f, 0.4f),
		.ormAndNormalStrength = glm::vec4(1.0f, 0.8f, 0.6f, 0.5f),
		.textureIndices = glm::uvec4(whiteIndex, blackIndex, whiteIndex, blackIndex),
	};
	const uint32_t debugMaterialIndex = addMaterial(debugMaterial);
	std::array selfTestInput = {
		textureIndices[0],
		textureIndices[1],
		debugMaterialIndex,
	};

	VmaBuffer inputBuffer({
		.allocator = &Vulkan::Instance->memoryAllocator,
		.strideSize = sizeof(selfTestInput),
		.bufferUsage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		.memoryUsage = VMA_MEMORY_USAGE_CPU_TO_GPU,
		.debugName = "Bindless self-test input",
	});
	inputBuffer.writeData(selfTestInput.data(), sizeof(selfTestInput));

	VmaBuffer outputBuffer({
		.allocator = &Vulkan::Instance->memoryAllocator,
		.strideSize = sizeof(glm::vec4) * 6,
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

	std::array<glm::vec4, 6> outputValues{};
	outputBuffer.readData(outputValues.data(), sizeof(outputValues));

	const auto approximately = [](float actual, float expected)
	{
		return std::abs(actual - expected) <= 0.01f;
	};
	for (uint32_t channel = 0; channel < 4; ++channel)
	{
		ASSERT_M(
			approximately(outputValues[0][channel], 1.0f),
			"Bindless self-test expected white channel %u, got %f",
			channel,
			outputValues[0][channel])
		ASSERT_M(
			approximately(outputValues[1][channel], 0.0f),
			"Bindless self-test expected black channel %u, got %f",
			channel,
			outputValues[1][channel])
		ASSERT_M(
			approximately(outputValues[2][channel], debugMaterial.baseColorFactor[channel]),
			"Bindless self-test expected material albedo channel %u to be %f, got %f",
			channel,
			debugMaterial.baseColorFactor[channel],
			outputValues[2][channel])
		ASSERT_M(
			approximately(outputValues[3][channel], debugMaterial.baseColorFactor[channel]),
			"Bindless self-test expected material base-color channel %u to be %f, got %f",
			channel,
			debugMaterial.baseColorFactor[channel],
			outputValues[3][channel])
		ASSERT_M(
			approximately(outputValues[4][channel], debugMaterial.emissiveFactorAndClipThreshold[channel]),
			"Bindless self-test expected material emissive/clip channel %u to be %f, got %f",
			channel,
			debugMaterial.emissiveFactorAndClipThreshold[channel],
			outputValues[4][channel])
		ASSERT_M(
			approximately(outputValues[5][channel], static_cast<float>(debugMaterial.textureIndices[channel])),
			"Bindless self-test expected material texture index channel %u to be %f, got %f",
			channel,
			static_cast<float>(debugMaterial.textureIndices[channel]),
			outputValues[5][channel])
	}
	removeMaterial(debugMaterialIndex);

	const BindlessTexture2DHandle reusedHandle =
		addTexture2D(blackImageView, samplerInfo);
	ASSERT(reusedHandle.index == temporaryIndex)
	ASSERT(reusedHandle.generation == temporaryGeneration + 1)
	ASSERT(temporaryHandle.generation != reusedHandle.generation)
	removeTexture2D(reusedHandle);

	outputBuffer.release();
	inputBuffer.release();
	LOG("Bindless resources self-test passed")
}
#endif

void BindlessResources::uploadMaterialTable()
{
	ASSERT(Instance == this)
	ASSERT(bindlessDescriptorSet.get() != VK_NULL_HANDLE)

	// avoid potential issues from synchronized gpu read & cpu write
	if (materialTableBuffer.buffer != VK_NULL_HANDLE) {
		Vulkan::Instance->waitDeviceIdle();
	}

	// material buffer needs to be recreated if it hasn't been created before, or if size doesn't match
	const uint32_t materialCapacity =
		static_cast<uint32_t>(std::max<size_t>(materialRecords.size(), 1));
	if (materialTableBuffer.buffer == VK_NULL_HANDLE || materialTableBuffer.numStrides != materialCapacity)
	{
		if (materialTableBuffer.buffer != VK_NULL_HANDLE) {
			materialTableBuffer.release();
		}
		materialTableBuffer = VmaBuffer({
			.allocator = &Vulkan::Instance->memoryAllocator,
			.strideSize = sizeof(glm::GpuMaterial),
			.bufferUsage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			.memoryUsage = VMA_MEMORY_USAGE_CPU_TO_GPU,
			.debugName = "Bindless material table buffer",
			.numStrides = materialCapacity,
		});
		bindlessDescriptorSet.pointToBuffer(
			materialTableBuffer,
			MaterialBufferBinding,
			VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
		ASSERT(materialTableBuffer.buffer != VK_NULL_HANDLE)
	}

	// actual write
	if (materialRecords.empty())
	{
		glm::GpuMaterial emptyMaterial{};
		materialTableBuffer.writeData(&emptyMaterial, sizeof(emptyMaterial));
	}
	else
	{
		materialTableBuffer.writeData(materialRecords.data(), sizeof(glm::GpuMaterial) * materialRecords.size());
	}
}

void BindlessResources::uploadGeometryRecordTable()
{
	ASSERT(Instance == this)
	ASSERT(bindlessDescriptorSet.get() != VK_NULL_HANDLE)

	if (geometryRecordTableBuffer.buffer != VK_NULL_HANDLE) {
		Vulkan::Instance->waitDeviceIdle();
	}

	const uint32_t geometryRecordCapacity =
		static_cast<uint32_t>(std::max<size_t>(geometryRecords.size(), 1));
	if (geometryRecordTableBuffer.buffer == VK_NULL_HANDLE ||
		geometryRecordTableBuffer.numStrides != geometryRecordCapacity)
	{
		if (geometryRecordTableBuffer.buffer != VK_NULL_HANDLE) {
			geometryRecordTableBuffer.release();
		}
		geometryRecordTableBuffer = VmaBuffer({
			.allocator = &Vulkan::Instance->memoryAllocator,
			.strideSize = sizeof(glm::GpuGeometryRecord),
			.bufferUsage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			.memoryUsage = VMA_MEMORY_USAGE_CPU_TO_GPU,
			.debugName = "Bindless geometry record table buffer",
			.numStrides = geometryRecordCapacity,
		});
		bindlessDescriptorSet.pointToBuffer(
			geometryRecordTableBuffer,
			GeometryRecordBufferBinding,
			VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
		ASSERT(geometryRecordTableBuffer.buffer != VK_NULL_HANDLE)
	}

	if (geometryRecords.empty())
	{
		glm::GpuGeometryRecord emptyGeometryRecord{};
		geometryRecordTableBuffer.writeData(&emptyGeometryRecord, sizeof(emptyGeometryRecord));
	}
	else
	{
		geometryRecordTableBuffer.writeData(
			geometryRecords.data(),
			sizeof(glm::GpuGeometryRecord) * geometryRecords.size());
	}
}

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
