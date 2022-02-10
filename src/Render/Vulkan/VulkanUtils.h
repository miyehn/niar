#pragma once
#include <string>
#include <vulkan/vulkan.h>
#include "Render/Vulkan/Vulkan.hpp"
#include "Utils/myn/Color.h"

class SDL_Window;

namespace vk
{
	bool init_window(
		const std::string& name,
		int width,
		int height,
		SDL_Window** window);

	void copyBuffer(VkBuffer dstBuffer, VkBuffer srcBuffer, VkDeviceSize size);

	void insertImageBarrier(
		VkCommandBuffer cmdbuf,
		VkImage image,
		VkImageSubresourceRange subresourceRange,
		VkPipelineStageFlags srcStageMask,
		VkPipelineStageFlags dstStageMask,
		VkAccessFlags srcAccessMask,
		VkAccessFlags dstAccessMask,
		VkImageLayout oldLayout,
		VkImageLayout newLayout);

	void insertBufferBarrier(
		VkCommandBuffer cmdbuf,
		VkBuffer buffer,
		VkPipelineStageFlags srcStageMask,
		VkPipelineStageFlags dstStageMask,
		VkAccessFlags srcAccessMask,
		VkAccessFlags dstAccessMask
		);

	void uploadPixelsToImage(
		uint8_t *pixels,
		int32_t offsetX,
		int32_t offsetY,
		uint32_t extentX,
		uint32_t extentY,
		uint32_t pixelSize,
		VmaAllocatedImage outResource);

	void generateMips(VmaAllocatedImage image, uint32_t width, uint32_t height);

	void blitToScreen(VkCommandBuffer cmdbuf, VkImage image, VkOffset3D srcOffsetMin, VkOffset3D srcOffsetMax);

	void drawFullscreenTriangle(VkCommandBuffer cmdbuf);
}

class ScopedDrawEvent
{
	VkCommandBuffer &cmdbuf;
public:
	ScopedDrawEvent(VkCommandBuffer &cmdbuf, const std::string &name, myn::Color color = {1, 1, 1, 1});
	~ScopedDrawEvent();
};

#ifdef WINOS
#define SCOPED_DRAW_EVENT(CMDBUF, NAME, ...) ScopedDrawEvent __scopedDrawEvent(CMDBUF, NAME, __VA_ARGS__);
#define DEBUG_LABEL(CMDBUF, NAME, ...) Vulkan::Instance->cmdInsertDebugLabel(CMDBUF, NAME, __VA_ARGS__);
#define NAME_OBJECT(VK_OBJECT_TYPE, OBJECT, NAME) Vulkan::Instance->setObjectName(VK_OBJECT_TYPE, (uint64_t)OBJECT, NAME);
#else
#define SCOPED_DRAW_EVENT(CMDBUF, NAME, ...) ;
#define DEBUG_LABEL(CMDBUF, NAME, ...) ;
#define NAME_OBJECT(VK_OBJECT_TYPE, OBJECT, NAME) ;
#endif
