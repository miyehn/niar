#include "Texture.h"
#include <stb_image/stb_image.h>
#include "Render/Vulkan/Vulkan.hpp"

std::unordered_map<std::string, Texture *> Texture::pool;

void Texture::cleanup()
{
	for (auto & it : Texture::pool)
	{
		delete it.second;
	}
}

Texture *Texture::get(const std::string &path)
{
	auto it = Texture::pool.find(path);
	if (it != Texture::pool.end()) return (*it).second;

	auto new_texture = new Texture2D(path);
	Texture::pool[path] = new_texture;
	return new_texture;
}

Texture::~Texture()
{
	vmaDestroyImage(Vulkan::Instance->memoryAllocator, resource.image, resource.allocation);
}

//--------

Texture2D::Texture2D(const std::string &path)
{
	num_slices = 1;

	int native_channels;
	stbi_uc* pixels = stbi_load(path.c_str(), &width, &height, &native_channels, STBI_rgb_alpha);
	LOG("load texture w %d h %d channels %d", width, height, native_channels)
	EXPECT(pixels != nullptr, true)

	VkDeviceSize imageSize = width * height * 4;
	VkFormat imageFormat = VK_FORMAT_R8G8B8A8_SRGB;

	VmaBuffer stagingBuffer(&Vulkan::Instance->memoryAllocator, imageSize,
							VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
	stagingBuffer.writeData(pixels);
	stbi_image_free(pixels);

	VkExtent3D imageExtent = {
		.width = static_cast<uint32_t>(width),
		.height = static_cast<uint32_t>(height),
		.depth = 1
	};

	VkImageCreateInfo imgInfo = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.imageType = VK_IMAGE_TYPE_2D,
		.format = imageFormat,
		.extent = imageExtent,
		.mipLevels = 1,
		.arrayLayers = 1,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.tiling = VK_IMAGE_TILING_OPTIMAL,
		.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT
	};

	VmaAllocationCreateInfo imgAllocInfo = {
		.usage = VMA_MEMORY_USAGE_GPU_ONLY
	};

	vmaCreateImage(Vulkan::Instance->memoryAllocator, &imgInfo, &imgAllocInfo, &resource.image, &resource.allocation, nullptr);

	Vulkan::Instance->immediateSubmit([&](VkCommandBuffer cmdbuf)
	{
		// image layouts
		auto transferLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		auto shaderReadLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		// barrier the image into the transfer-receive layout
		VkImageSubresourceRange range = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.baseMipLevel = 0,
			.levelCount = 1,
			.baseArrayLayer = 0,
			.layerCount = 1
		};
		VkImageMemoryBarrier imageBarrier_toTransfer = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			.srcAccessMask = 0,
			.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
			.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.newLayout = transferLayout,
			.image = resource.image,
			.subresourceRange = range,
		};
		vkCmdPipelineBarrier(
			cmdbuf,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			0,
			0,
			nullptr,
			0,
			nullptr,
			1,
			&imageBarrier_toTransfer);

		// do the transfer
		VkBufferImageCopy copyRegion = {
			.bufferOffset = 0,
			.bufferRowLength = 0,
			.bufferImageHeight = 0,
			.imageSubresource = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.mipLevel = 0,
				.baseArrayLayer = 0,
				.layerCount = 1
			},
			.imageOffset = {0, 0, 0},
			.imageExtent = imageExtent
		};
		vkCmdCopyBufferToImage(
			cmdbuf,
			stagingBuffer.getBufferInstance(),
			resource.image,
			transferLayout,
			1,
			&copyRegion);

		// barrier it again into shader readonly optimal layout
		VkImageMemoryBarrier imageBarrier_toShaderReadonly = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
			.dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
			.oldLayout = transferLayout,
			.newLayout = shaderReadLayout,
			.image = resource.image,
			.subresourceRange = range,
		};
		vkCmdPipelineBarrier(
			cmdbuf,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			0,
			0,
			nullptr,
			0,
			nullptr,
			1,
			&imageBarrier_toShaderReadonly);

	});

	stagingBuffer.release();

	// create image view
	VkImageViewCreateInfo viewInfo = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.flags = 0,
		.image = resource.image,
		.viewType = VK_IMAGE_VIEW_TYPE_2D,
		.format = imageFormat,
		.components = {VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
		.subresourceRange = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.baseMipLevel = 0,
			.levelCount = 1,
			.baseArrayLayer = 0,
			.layerCount = 1
		}
	};
	EXPECT(vkCreateImageView(Vulkan::Instance->device, &viewInfo, nullptr, &imageView), VK_SUCCESS)
}

Texture2D::~Texture2D()
{
	vkDestroyImageView(Vulkan::Instance->device, imageView, nullptr);
}
