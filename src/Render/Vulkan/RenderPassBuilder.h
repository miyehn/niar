#pragma once
#include <vulkan/vulkan.h>
#include <vector>

class Vulkan;

struct RenderPassBuilder
{
	RenderPassBuilder() = default;

	std::vector<VkAttachmentDescription> colorAttachments;
	VkAttachmentDescription depthAttachment;
	std::vector<VkSubpassDescription> subpasses;
	std::vector<VkSubpassDependency> dependencies;
	bool useDepthAttachment;

	VkRenderPass build(Vulkan* vulkan);
	VkRenderPass buildDisplayPass(Vulkan* vulkan, VkFormat colorFormat);
};
