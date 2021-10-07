#include "Utils/myn/Log.h"
#include "RenderPassBuilder.h"

RenderPassBuilder::RenderPassBuilder(VkFormat colorFormat, VkFormat depthFormat)
{
	//-------- Render pass (TODO: abstract away) --------

	// a render pass: load the attachment, r/w operations (by subpasses), then release it?
	// renderpass -> subpass -> attachmentReferences -> attachment
	// a render pass holds ref to an array of color attachments.
	// A subpass then use an array of attachmentRef to selectively get the attachments and use them
	colorAttachment = {
		.format = colorFormat,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED, // layout when attachment is loaded
		.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
	};
	colorAttachmentRef = {
		// matches layout(location=X)
		.attachment = 0,
		// "which layout we'd like it to have during this subpass"
		// so, initialLayout -> this -> finalLayout ?
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};
	depthAttachment = {
		.format = depthFormat,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
	};
	depthAttachmentRef = {
		.attachment = 1,
		.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
	};
	subpass = {
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.colorAttachmentCount = 1,
		.pColorAttachments = &colorAttachmentRef, // an array, index matches layout (location=X) out vec4 outColor
		.pDepthStencilAttachment = &depthAttachmentRef
	};
	dependency = {
		.srcSubpass = VK_SUBPASS_EXTERNAL,
		.dstSubpass = 0,
		// wait for the swap chain to finish reading
		// Question: first access scope includes color_attachment_output as well?? (Isn't it just read by the monitor?)
		.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		// before we can write to it
		.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		.srcAccessMask = 0,
		.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
	};
}

VkRenderPass RenderPassBuilder::build(VkDevice &device)
{
	VkAttachmentDescription attachments[2] = { colorAttachment, depthAttachment };
	VkRenderPassCreateInfo renderPassInfo = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.attachmentCount = 2,
		.pAttachments = attachments,
		.subpassCount = 1,
		.pSubpasses = &subpass,
		.dependencyCount = 1,
		.pDependencies = &dependency
	};
	VkRenderPass renderPass;
	EXPECT(vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass), VK_SUCCESS)
	return renderPass;
}
