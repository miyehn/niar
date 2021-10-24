#include <SDL2/SDL.h>
#include "VulkanUtils.h"
#include "Vulkan.hpp"

Vulkan* Vulkan::Instance = nullptr;

bool vk::init_window(
	const std::string &name,
	int width,
	int height,
	SDL_Window **window,
	int *drawable_width,
	int *drawable_height)
{
	if (Vulkan::Instance) return true;

	SDL_Init(SDL_INIT_VIDEO);

	// create window
	*window = SDL_CreateWindow(
		name.c_str(),
		100, 100, // SDL_WINDOWPOS_UNDEFINED, or SDL_WINDOWPOS_CENTERED
		width, height, // specify window size
		SDL_WINDOW_VULKAN
	);
	if (*window == nullptr)
	{
		ERR("Error creating SDL window: %s", SDL_GetError());
		return false;
	}

	Vulkan::Instance = new Vulkan(*window);

	// TODO: get drawable size (vulkan)?
	*drawable_width = width;
	*drawable_height = height;

	return true;
}

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

void vk::blitToScreen(VkCommandBuffer cmdbuf, VkImage image, VkOffset3D srcOffsetMin, VkOffset3D srcOffsetMax)
{
	VkImage swapChainImage = Vulkan::Instance->getCurrentSwapChainImage();
	VkExtent2D swapChainExtent = Vulkan::Instance->swapChainExtent;

	// barrier the swapchain image into transfer-dst layout
	vk::insertImageBarrier(cmdbuf, swapChainImage,
						   {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
						   VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
						   VK_PIPELINE_STAGE_TRANSFER_BIT,
						   0,
						   VK_ACCESS_TRANSFER_WRITE_BIT,
						   VK_IMAGE_LAYOUT_UNDEFINED,
						   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	// blit to screen
	VkOffset3D dstOffsetMin = {0, 0, 0};
	VkOffset3D dstOffsetMax = {
		static_cast<int32_t>(swapChainExtent.width),
		static_cast<int32_t>(swapChainExtent.height),
		1
	};
	VkImageBlit blitRegion = {
		.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1},
		.srcOffsets = { srcOffsetMin, srcOffsetMax },
		.dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1},
		.dstOffsets = { dstOffsetMin, dstOffsetMax }
	};
	vkCmdBlitImage(cmdbuf,
				   image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				   swapChainImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				   1, &blitRegion,
				   VK_FILTER_LINEAR);

	// barrier swapchain image back to present optimal
	vk::insertImageBarrier(cmdbuf, swapChainImage,
						   {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
						   VK_PIPELINE_STAGE_TRANSFER_BIT,
						   VK_PIPELINE_STAGE_TRANSFER_BIT,
						   VK_ACCESS_TRANSFER_WRITE_BIT,
						   VK_ACCESS_MEMORY_READ_BIT,
						   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
						   VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
}

void vk::draw_fullscreen_triangle(VkCommandBuffer cmdbuf)
{
	vkCmdDraw(cmdbuf, 3, 1, 0, 0);
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
