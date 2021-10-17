#include "VulkanUtils.h"
#include "Vulkan.hpp"

void vk::copyBuffer(VkBuffer dstBuffer, VkBuffer srcBuffer, VkDeviceSize size)
{
	Vulkan::Instance->immediateSubmit([&](VkCommandBuffer cmdbuf)
	{
		VkBufferCopy copyRegion = {
			.srcOffset = 0,
			.dstOffset = 0,
			.size = size
		};
		vkCmdCopyBuffer(cmdbuf, srcBuffer, dstBuffer, 1, &copyRegion);
	});
}
void vk::insertImageBarrier(VkCommandBuffer cmdbuf,
							VkImage image,
							VkImageSubresourceRange subresourceRange,
							VkPipelineStageFlags srcStageMask,
							VkPipelineStageFlags dstStageMask,
							VkAccessFlags srcAccessMask,
							VkAccessFlags dstAccessMask,
							VkImageLayout oldLayout,
							VkImageLayout newLayout)
{
	VkImageMemoryBarrier imageBarrier = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.srcAccessMask = srcAccessMask,
		.dstAccessMask = dstAccessMask,
		.oldLayout = oldLayout,
		.newLayout = newLayout,
		// below 2: used for transferring exclusive queue family ownership (otherwise fine to ignore)
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = image,
		.subresourceRange = subresourceRange
	};
	vkCmdPipelineBarrier(
		cmdbuf,
		srcStageMask,
		dstStageMask,
		0,
		0,
		nullptr,
		0,
		nullptr,
		1,
		&imageBarrier);
}

ScopedDrawEvent::ScopedDrawEvent(VkCommandBuffer &cmdbuf, const std::string &name, myn::Color color) : cmdbuf(cmdbuf)
{
	VkDebugUtilsLabelEXT markerInfo {
		.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
		.pLabelName = name.c_str(),
		.color = {color.r, color.g, color.b, color.a}
	};
	Vulkan::Instance->fn_vkCmdBeginDebugUtilsLabelEXT(cmdbuf, &markerInfo);
}

ScopedDrawEvent::~ScopedDrawEvent()
{
	Vulkan::Instance->fn_vkCmdEndDebugUtilsLabelEXT(cmdbuf);
}
