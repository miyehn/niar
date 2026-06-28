#pragma once

#include "Render/Vulkan/DescriptorSet.h"
#include "cshared_common.h"

#include <array>
#include <vector>
#include <vulkan/vulkan.h>

#define TMP_BINDLESS_DEBUG 1

struct BindlessTexture2DHandle
{
	uint32_t index = INVALID_BINDLESS_INDEX;
	uint32_t generation = 0;
};

struct BindlessTexture2DInfo
{
	bool registerTexture = false;
	VkSamplerCreateInfo samplerInfo;
};

class BindlessResources
{
public:
	static BindlessResources* Instance;

	BindlessResources() = default;
	BindlessResources(const BindlessResources&) = delete;
	BindlessResources& operator=(const BindlessResources&) = delete;

	void init();
	void release();

	BindlessTexture2DHandle addTexture2D(
		VkImageView imageView,
		const VkSamplerCreateInfo& samplerInfo);
	void removeTexture2D(BindlessTexture2DHandle handle);

	uint32_t addMaterial(const glm::GpuMaterial& material);
	void updateMaterial(uint32_t index, const glm::GpuMaterial& material);
	void removeMaterial(uint32_t index);
	void clearMaterials();

	// Returns the shader-visible index. Invalid or stale handles are errors.
	uint32_t validate(BindlessTexture2DHandle handle) const;

	const DescriptorSet& descriptorSet() const { return bindlessDescriptorSet; }
	const DescriptorSetLayout& layout() const { return bindlessSetLayout; }

	// this can stay as a safety feature
	void setTexture2DFiller(
		VkImageView imageView,
		const VkSamplerCreateInfo& samplerInfo);

#if TMP_BINDLESS_DEBUG
	uint32_t occupiedTexture2DSlotCount() const;
	uint32_t activeMaterialCount() const;
	void assertMaterialIndexOccupied(uint32_t index) const;

	// todo [myn][bindless]: this is to be removed in later phases of bindless
	void runDebugSelfTest(
		BindlessTexture2DHandle whiteHandle,
		BindlessTexture2DHandle blackHandle,
		VkImageView blackImageView,
		const VkSamplerCreateInfo& samplerInfo);
#endif

private:

	VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
	DescriptorSetLayout bindlessSetLayout;
	DescriptorSet bindlessDescriptorSet;

	// this can stay as a safety feature
	VkDescriptorImageInfo fillerTexture2DDescriptor{};

	// bindless textures
	struct Texture2DSlot {
		uint32_t generation = 0;
		bool occupied = false;
	};
	std::array<Texture2DSlot, MAX_BINDLESS_TEXTURES_2D> texture2DSlots{};
	std::vector<uint32_t> freeTexture2DSlots;

	// material table
	VmaBuffer materialTableBuffer;
	std::vector<glm::GpuMaterial> materialRecords; // data that gets uploaded to materialTableBuffer
	std::vector<uint8_t> materialSlotOccupied;
	std::vector<uint32_t> freeMaterialSlots;
	void uploadMaterialTable();
};
