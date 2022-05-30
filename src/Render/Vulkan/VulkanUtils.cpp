#include <SDL2/SDL.h>
#include <glm/common.hpp>
#include "VulkanUtils.h"
#include "Vulkan.hpp"

Vulkan* Vulkan::Instance = nullptr;

bool vk::init_window(
	const std::string &name,
	int width,
	int height,
	SDL_Window **window)
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

void vk::insertBufferBarrier(
	VkCommandBuffer cmdbuf,
	VkBuffer buffer,
	VkPipelineStageFlags srcStageMask,
	VkPipelineStageFlags dstStageMask,
	VkAccessFlags srcAccessMask,
	VkAccessFlags dstAccessMask
) {
	VkBufferMemoryBarrier bufferBarrier = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
		.srcAccessMask = srcAccessMask,
		.dstAccessMask = dstAccessMask,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.buffer = buffer,
		.offset = 0,
		.size = VK_WHOLE_SIZE
	};
	vkCmdPipelineBarrier(
		cmdbuf,
		srcStageMask,
		dstStageMask,
		0,
		0,
		nullptr,
		1,
		&bufferBarrier,
		0,
		nullptr);
}

void vk::blitToScreen(VkCommandBuffer cmdbuf, VkImage image, VkOffset3D srcOffsetMin, VkOffset3D srcOffsetMax)
{
	VkImage swapChainImage = Vulkan::Instance->getCurrentSwapChainImage();
	VkExtent2D swapChainExtent = Vulkan::Instance->swapChainExtent;

	// barrier the swapchain image into transfer-dst layout
	vk::insertImageBarrier(cmdbuf, swapChainImage,
						   {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
						   VK_PIPELINE_STAGE_TRANSFER_BIT,
						   VK_PIPELINE_STAGE_TRANSFER_BIT,
						   VK_ACCESS_TRANSFER_READ_BIT,
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

void vk::drawFullscreenTriangle(VkCommandBuffer cmdbuf)
{
	vkCmdDraw(cmdbuf, 3, 1, 0, 0);
}

void vk::uploadPixelsToImage(
	uint8_t *pixels,
	int32_t offsetX,
	int32_t offsetY,
	uint32_t extentX,
	uint32_t extentY,
	uint32_t pixelSize,
	VmaAllocatedImage outResource)
{
	VkDeviceSize copyRegionSize = extentX * extentY * pixelSize;
	VmaBuffer stagingBuffer(&Vulkan::Instance->memoryAllocator, copyRegionSize,
							VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
	stagingBuffer.writeData(pixels);
	Vulkan::Instance->immediateSubmit(
		[&](VkCommandBuffer cmdbuf)
		{
			// image layout
			auto transferLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			auto shaderReadLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			// barrier the image into the transfer-receive layout
			vk::insertImageBarrier(
				cmdbuf,
				outResource.image,
				{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0,1},
				VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				0,
				VK_ACCESS_TRANSFER_WRITE_BIT,
				VK_IMAGE_LAYOUT_UNDEFINED,
				transferLayout);

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
				.imageOffset = {offsetX, offsetY, 0},
				.imageExtent = {extentX, extentY, 1}
			};
			vkCmdCopyBufferToImage(
				cmdbuf,
				stagingBuffer.getBufferInstance(),
				outResource.image,
				transferLayout,
				1,
				&copyRegion);

			//barrier it again into shader readonly optimal layout
			vk::insertImageBarrier(
				cmdbuf,
				outResource.image,
				{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0,1},
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
				VK_ACCESS_TRANSFER_WRITE_BIT,
				VK_ACCESS_SHADER_READ_BIT,
				transferLayout,
				shaderReadLayout);
		});

	stagingBuffer.release();
}

// took from ImGui
static inline bool isPowerOfTwo(uint32_t v) {
	return v != 0 && (v & (v - 1)) == 0;
}

void vk::generateMips(VmaAllocatedImage image, uint32_t width, uint32_t height)
{
	if (!isPowerOfTwo(width) || !isPowerOfTwo(height))
	{
		WARN("Trying to generate an image that doesn't have power of 2 dimensions %ux%u. skipping..", width, height)
		return;
	}

	uint32_t numMips = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;
	Vulkan::Instance->immediateSubmit(
		[&](VkCommandBuffer cmdbuf)
		{
			for (uint32_t i = 1; i < numMips; i++)
			{
				// prepare source
				vk::insertImageBarrier(
					cmdbuf, image.image,
					{VK_IMAGE_ASPECT_COLOR_BIT, i - 1, 1, 0, 1},
					VK_PIPELINE_STAGE_TRANSFER_BIT,
					VK_PIPELINE_STAGE_TRANSFER_BIT,
					VK_ACCESS_TRANSFER_WRITE_BIT,
					VK_ACCESS_TRANSFER_READ_BIT,
					i == 1 ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
				// prepare dest
				vk::insertImageBarrier(
					cmdbuf, image.image,
					{VK_IMAGE_ASPECT_COLOR_BIT, i, 1, 0, 1},
					VK_PIPELINE_STAGE_TRANSFER_BIT,
					VK_PIPELINE_STAGE_TRANSFER_BIT,
					VK_ACCESS_TRANSFER_READ_BIT,
					VK_ACCESS_TRANSFER_WRITE_BIT,
					VK_IMAGE_LAYOUT_UNDEFINED,
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
					);
				VkOffset3D offsetMin = {0, 0, 0};
				VkOffset3D srcOffsetMax = {
					static_cast<int>(std::max(1u, width >> (i-1))),
					static_cast<int>(std::max(1u, height >> (i-1))),
					1
				};
				VkOffset3D dstOffsetMax = {
					static_cast<int>(std::max(1u, width >> i)),
					static_cast<int>(std::max(1u, height >> i)),
					1
				};
				VkImageBlit blitRegion = {
					.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, i - 1, 0, 1},
					.srcOffsets = { offsetMin, srcOffsetMax },
					.dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, i, 0, 1},
					.dstOffsets = { offsetMin, dstOffsetMax }
				};
				vkCmdBlitImage(cmdbuf,
							   image.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
							   image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
							   1, &blitRegion,
							   VK_FILTER_LINEAR);

			}
			// get the levels back to shader readonly optimal
			vk::insertImageBarrier(
				cmdbuf, image.image,
				{VK_IMAGE_ASPECT_COLOR_BIT, 0, numMips-1, 0, 1},
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_ACCESS_TRANSFER_WRITE_BIT,
				VK_ACCESS_TRANSFER_READ_BIT,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			);
			vk::insertImageBarrier(
				cmdbuf, image.image,
				{VK_IMAGE_ASPECT_COLOR_BIT, numMips-1, 1, 0, 1},
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_ACCESS_TRANSFER_WRITE_BIT,
				VK_ACCESS_TRANSFER_READ_BIT,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			);
		});
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
