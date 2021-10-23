#pragma once
#include <vulkan/vulkan.h>
#include <string>
#include "Utils/myn/Color.h"

namespace vk
{
	void copyBuffer(VkBuffer dstBuffer, VkBuffer srcBuffer, VkDeviceSize size);

	void insertImageBarrier(VkCommandBuffer cmdbuf,
							VkImage image,
							VkImageSubresourceRange subresourceRange,
							VkPipelineStageFlags srcStageMask,
							VkPipelineStageFlags dstStageMask,
							VkAccessFlags srcAccessMask,
							VkAccessFlags dstAccessMask,
							VkImageLayout oldLayout,
							VkImageLayout newLayout);

	void blitToScreen(VkCommandBuffer cmdbuf, VkImage image, VkOffset3D srcOffsetMin, VkOffset3D srcOffsetMax);

	void draw_fullscreen_triangle(VkCommandBuffer cmdbuf);
}

class ScopedDrawEvent
{
	VkCommandBuffer &cmdbuf;
public:
	ScopedDrawEvent(VkCommandBuffer &cmdbuf, const std::string &name, myn::Color color = {1, 1, 1, 1});
	~ScopedDrawEvent();
};

#define SCOPED_DRAW_EVENT(CMDBUF, NAME, ...) ScopedDrawEvent __scopedDrawEvent(CMDBUF, NAME, __VA_ARGS__);

#define DEBUG_LABEL(CMDBUF, NAME, ...) Vulkan::Instance->cmdInsertDebugLabel(CMDBUF, NAME, __VA_ARGS__);

#define NAME_OBJECT(VK_OBJECT_TYPE, OBJECT, NAME) Vulkan::Instance->setObjectName(VK_OBJECT_TYPE, (uint64_t)OBJECT, NAME);
