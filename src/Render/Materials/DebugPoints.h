#pragma once
#include "Material.h"

class DeferredRenderer;

struct PointData
{
	explicit PointData(const glm::vec3& in_position) : position(in_position) {};
	glm::vec3 position;
	float _pad = 0.0f;

	// about how to cut the binding-th array into strides
	static void getBindingDescription(VkVertexInputBindingDescription& bindingDescription)
	{
		bindingDescription = {
			.binding = 0, // binding index. Just one binding if all vertex data is passed as one array
			.stride = sizeof(PointData),
			.inputRate = VK_VERTEX_INPUT_RATE_VERTEX, // data is per-vertex (as opposed to per-instance)
		};
	}

	// about how to interpret each stride
	static void getAttributeDescriptions(std::vector<VkVertexInputAttributeDescription>& attributeDescriptions)
	{
		attributeDescriptions.clear();
		attributeDescriptions.push_back(VkVertexInputAttributeDescription{
			.location = 0,
			.binding = 0,
			.format = VK_FORMAT_R32G32B32_SFLOAT,
			.offset = (uint32_t)offsetof(PointData, position)
		});
	}
};

// behaves like a material because it has its own pipeline, etc., but is actually drawing from a vertex buffer
// should be a singleton?
class DebugPoints
{
public:

	explicit DebugPoints(DeferredRenderer* renderer, const std::vector<PointData>& initialPoints);
	~DebugPoints();

	void bindAndDraw(VkCommandBuffer cmdbuf, std::vector<DescriptorSetBindingSlot> sharedDescriptorSets);

	static void cleanup();

	VmaBuffer pointsBuffer; // a vertex buffer

	uint32_t numPoints;

private:

	static VkPipeline pipeline;
	static VkPipelineLayout pipelineLayout;
};

