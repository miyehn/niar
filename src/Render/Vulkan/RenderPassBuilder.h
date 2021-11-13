#pragma once
#include <vulkan/vulkan.h>
#include <vector>

struct RenderPassBuilder
{
	RenderPassBuilder() = default;

	std::vector<VkAttachmentDescription> colorAttachments;
	VkAttachmentDescription depthAttachment;
	std::vector<VkSubpassDescription> subpasses;
	std::vector<VkSubpassDependency> dependencies;
	bool useDepthAttachment;

	VkRenderPass build(VkDevice &device);
	VkRenderPass buildDisplayPass(VkDevice &device, VkFormat colorFormat, VkFormat depthFormat);
};
