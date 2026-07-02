#pragma once
#include <vector>
#include <functional>
#include <vulkan/vulkan.h>
#include "DescriptorSet.h"
#include "Assets/Asset.h"
#include "Assets/ShaderModuleAsset.h"
#include <string>

struct PipelineState
{
	PipelineState();

	void setExtent(uint32_t width, uint32_t height);

	VkGraphicsPipelineCreateInfo getPipelineInfoTemplate();

	VkExtent2D targetExtent{};

	// vertex input
	bool useVertexInput;
	VkVertexInputBindingDescription bindingDescription{};
	std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};

	// input assembly
	bool useInputAssembly;
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{};

	// viewport
	bool useViewport;
	VkViewport viewport{};
	VkRect2D scissor{};
	VkPipelineViewportStateCreateInfo viewportInfo{};

	// rasterization
	bool useRasterization;
	VkPipelineRasterizationStateCreateInfo rasterizationInfo{};

	// multisampling (along edges for anti aliasing)
	bool useMultisampling;
	VkPipelineMultisampleStateCreateInfo multisamplingInfo{};

	// depth stencil
	bool useDepthStencil;
	VkPipelineDepthStencilStateCreateInfo depthStencilInfo{};

	// color blending
	bool useColorBlending;
	VkPipelineColorBlendAttachmentState colorBlendAttachmentInfo{};
	VkPipelineColorBlendStateCreateInfo colorBlendInfo{};
	// Override for multiple attachments — stored by value so pointers remain valid for deferred builds.
	// When non-empty, colorBlendInfo.attachmentCount and pAttachments are set from this vector.
	std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments;

	// dynamic state
	bool useDynamicState;
	VkPipelineDynamicStateCreateInfo dynamicStateInfo{};
};

struct GraphicsPipelineBuilder
{
	GraphicsPipelineBuilder() = default;

	void useDescriptorSetLayout(uint32_t setIndex, const DescriptorSetLayout &setLayout);

	void usePushConstantRange(const VkPushConstantRange& range);

	void build(VkPipeline &outPipeline, VkPipelineLayout &outPipelineLayout, const std::string& debugName);

	ShaderModuleDef vertDef = {"", "main", SS_Vertex, {}};
	ShaderModuleDef fragDef = {"", "main", SS_Fragment, {}};

	PipelineState pipelineState{};

	VkRenderPass compatibleRenderPass = VK_NULL_HANDLE;
	uint32_t compatibleSubpass = 0;

private:
	std::vector<DescriptorSetLayout> descriptorSetLayouts;
	std::vector<VkPushConstantRange> pushConstantRanges;
};

struct ComputePipelineBuilder
{
	ComputePipelineBuilder() = default;

	void useDescriptorSetLayout(uint32_t setIndex, const DescriptorSetLayout &setLayout);
	void usePushConstantRange(const VkPushConstantRange& range);

	void build(VkPipeline &outPipeline, VkPipelineLayout &outPipelineLayout, const std::string& debugName);

	ShaderModuleDef shaderDef = {"", "main", SS_Compute, {}};

private:
	std::vector<DescriptorSetLayout> descriptorSetLayouts;
	std::vector<VkPushConstantRange> pushConstantRanges;
};

// usage (for now): construct, set descriptor set, add shader paths, add groups, build
struct RayTracingPipelineBuilder
{
	RayTracingPipelineBuilder() = default;

	void useDescriptorSetLayout(uint32_t setIndex, const DescriptorSetLayout &setLayout);

	void build(VkPipeline &outPipeline, VkPipelineLayout &outPipelineLayout, const std::string& debugName);

	ShaderModuleDef rgenPath = {"", "main", SS_RayGen, {}};
	std::vector<ShaderModuleDef> rchitDefs;
	std::vector<ShaderModuleDef> rahitDefs;
	std::vector<ShaderModuleDef> rmissDefs;

	struct HitGroup {
		int rchitIndex, rahitIndex;
	};
	std::vector<HitGroup> hitGroups;

private:
	std::vector<DescriptorSetLayout> descriptorSetLayouts;
};


struct PipelineBase {
	VkPipeline pipeline = VK_NULL_HANDLE;
	VkPipelineLayout layout = VK_NULL_HANDLE;
	PipelineBase() = default;
	PipelineBase(const PipelineBase&) = delete; // delete copy constructor
	bool operator==(const PipelineBase& rhs) const {
		return this->pipeline == rhs.pipeline && this->layout == rhs.layout;
	}
	bool operator<(const PipelineBase& rhs) const {
		// sort order: group material pipelines of the same layout together
		if (this->layout != rhs.layout) return this->layout < rhs.layout;
		return this->pipeline < rhs.pipeline;
	}
	bool valid() const;

	virtual void build(const std::string& debugName) = 0;
	void destroy();

	virtual ~PipelineBase() {
		for (auto id : _callbackIds) Asset::unregister_callback(id);
		destroy();
	}

protected:
	std::vector<uint32_t> _callbackIds;
};


// Owning wrapper around a graphics pipeline.
struct GraphicsPipeline: PipelineBase
{
	GraphicsPipelineBuilder builder = {};
	// also registers shader-reload callbacks.
	void build(const std::string& debugName) override;
};

// Owning wrapper around a compute pipeline.
struct ComputePipeline: PipelineBase
{
	ComputePipelineBuilder builder = {};
	// also registers shader-reload callbacks.
	void build(const std::string& debugName) override;
};

// Owning wrapper around a ray tracing pipeline.
// onRebuilt is called after a shader-driven rebuild (e.g. to rebuild the ShaderBindingTable).
struct RayTracingPipeline: PipelineBase
{
	RayTracingPipelineBuilder builder = {};

	std::function<void()> onRebuilt;

	// also registers shader-reload callbacks.
	void build(const std::string& debugName) override;
};
