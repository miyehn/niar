#pragma once
#include <vulkan/vulkan.h>

struct RenderPassBuilder
{
	RenderPassBuilder(VkFormat colorFormat, VkFormat depthFormat);
	VkAttachmentDescription colorAttachment;
	VkAttachmentReference colorAttachmentRef;
	VkAttachmentDescription depthAttachment;
	VkAttachmentReference depthAttachmentRef;
	VkSubpassDescription subpass;
	VkSubpassDependency dependency;
	VkRenderPass build(VkDevice &device);
};
