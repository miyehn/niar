#include "ImageCreator.h"
#include "VulkanUtils.h"

ImageCreator::ImageCreator(
	VkFormat format,
	VkExtent3D extent,
	VkImageUsageFlags imageUsage,
	VkImageAspectFlags aspectMask,
	const std::string &debugName) : debugName(debugName)
{
	imageInfo = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.imageType = VK_IMAGE_TYPE_2D,
		.format = format,
		.extent = extent,
		.mipLevels = 1,
		.arrayLayers = 1,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.tiling = VK_IMAGE_TILING_OPTIMAL,
		.usage = imageUsage
	};
	allocInfo = {
		.usage = VMA_MEMORY_USAGE_GPU_ONLY,
		.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
	};
	viewInfo = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.viewType = VK_IMAGE_VIEW_TYPE_2D,
		.format = format,
		.subresourceRange = {aspectMask, 0, 1, 0, 1}
	};
}

void ImageCreator::create(VmaAllocatedImage &image, VkImageView &imageView)
{
	EXPECT(vmaCreateImage(
		Vulkan::Instance->memoryAllocator,
		&imageInfo, &allocInfo,
		&image.image, &image.allocation,
		nullptr), VK_SUCCESS)

	viewInfo.image = image.image;
	EXPECT(vkCreateImageView(Vulkan::Instance->device, &viewInfo, nullptr, &imageView), VK_SUCCESS)

	if (debugName.length() > 0)
	{
		NAME_OBJECT(VK_OBJECT_TYPE_IMAGE, image.image, debugName)
	}
}
