#pragma once
#include "Render/Materials/Material.h"

class DeferredRenderer;

struct PointData
{
	explicit PointData(const glm::vec3& in_position, const glm::u8vec4 in_color)
		: position(in_position), color(in_color) {};
	glm::vec3 position;
	glm::u8vec4 color;

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
		attributeDescriptions.push_back(VkVertexInputAttributeDescription{
			.location = 1,
			.binding = 0,
			.format = VK_FORMAT_R8G8B8A8_UNORM,
			.offset = (uint32_t)offsetof(PointData, color)
		});
	}
};

// behaves like a material because it has its own pipeline, etc., but is actually drawing from a vertex buffer
// usage: create an instance, add a bunch of points then call uploadVertexBuffer(), then call bindAndDraw() in a render pass
class DebugPoints
{
public:

	explicit DebugPoints(const DescriptorSetLayout& frameGlobalSetLayout, VkRenderPass compatiblePass, int compatibleSubpass=0);
	~DebugPoints();

	void addPoint(const glm::vec3& position, glm::u8vec4 color);

	void uploadVertexBuffer();

	void bindAndDraw(VkCommandBuffer cmdbuf);

	VmaBuffer pointsBuffer; // a vertex buffer potentially modified by compute shaders

	uint32_t numPoints() { return points.size(); }

	VkPipelineLayout getPipelineLayout() const { return pipelineLayout; }

private:

	VkPipeline pipeline = VK_NULL_HANDLE;
	VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
	//DescriptorSet descriptorSet = {};

	std::vector<PointData> points;
};

class DebugLines
{
public:

	explicit DebugLines(const DescriptorSetLayout& frameGlobalSetLayout, VkRenderPass compatiblePass, int compatibleSubpass=0);
	~DebugLines();

	void clear() { points.clear(); }

	void addSegment(const PointData& endPoint1, const PointData& endPoint2);

	void addBox(const glm::vec3& minPos, const glm::vec3& maxPos, glm::u8vec4 color);

	void uploadVertexBuffer();

	void bindAndDraw(VkCommandBuffer cmdbuf);

	VmaBuffer pointsBuffer; // a vertex buffer potentially modified by compute shaders

	uint32_t numSegments() { return points.size() / 2; }

	VkPipelineLayout getPipelineLayout() const { return pipelineLayout; }

private:
	VkPipeline pipeline = VK_NULL_HANDLE;
	VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
	//DescriptorSet descriptorSetPtr = {};

	std::vector<PointData> points;
};