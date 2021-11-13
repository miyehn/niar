#include "Utils/myn/Log.h"
#include "RenderPassBuilder.h"

VkRenderPass RenderPassBuilder::build(VkDevice &device)
{
	std::vector<VkAttachmentDescription> attachments;
	for (auto colorAttachment : colorAttachments)
	{
		attachments.push_back(colorAttachment);
	}
	if (useDepthAttachment) attachments.push_back(depthAttachment);
	VkRenderPassCreateInfo renderPassInfo = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.attachmentCount = static_cast<uint32_t>(attachments.size()),
		.pAttachments = attachments.data(),
		.subpassCount = static_cast<uint32_t>(subpasses.size()),
		.pSubpasses = subpasses.data(),
		.dependencyCount = static_cast<uint32_t>(dependencies.size()),
		.pDependencies = dependencies.data()
	};
	VkRenderPass renderPass;
	EXPECT(vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass), VK_SUCCESS)
	return renderPass;
}

VkRenderPass RenderPassBuilder::buildDisplayPass(VkDevice &device, VkFormat colorFormat, VkFormat depthFormat)
{
	// a render pass: load the attachment, r/w operations (by subpasses), then release it?
	// renderpass -> subpass -> attachmentReferences -> attachment
	// a render pass holds ref to an array of color attachments.
	// A subpass then use an array of attachmentRef to selectively get the attachments and use them
	colorAttachments.push_back(
		{
			.format = colorFormat,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			//.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, // layout when attachment is loaded
			.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
		});

	VkAttachmentReference colorAttachmentRef = {
		// matches layout(location=X)
		.attachment = 0,
		// "which layout we'd like it to have during this subpass"
		// so, initialLayout -> this -> finalLayout ?
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};

	subpasses.push_back(
		{
			.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
			.colorAttachmentCount = 1,
			.pColorAttachments = &colorAttachmentRef, // an array, index matches layout (location=X) out vec4 outColor
			.pDepthStencilAttachment = nullptr
		});
	dependencies.push_back(
		{
			.srcSubpass = VK_SUBPASS_EXTERNAL,
			.dstSubpass = 0,
			// wait for the swap chain to finish reading
			// Question: first access scope includes color_attachment_output as well?? (Isn't it just read by the monitor?)
			.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			// before we can write to it
			.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.srcAccessMask = 0,
			.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		});

	useDepthAttachment = false;
	return build(device);
}
