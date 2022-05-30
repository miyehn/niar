#pragma once
#include "Vulkan.hpp"

struct ImageCreator
{
	ImageCreator(
		VkFormat format,
		VkExtent3D extent,
		VkImageUsageFlags imageUsage,
		VkImageAspectFlags aspectMask,
		const std::string &debugName = "");

	VkImageCreateInfo imageInfo;
	VmaAllocationCreateInfo allocInfo;
	VkImageViewCreateInfo viewInfo;
	const std::string debugName;

	void create(VmaAllocatedImage& image, VkImageView &imageView);
};
