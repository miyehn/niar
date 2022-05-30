#pragma once
#include <glm/glm.hpp>
#if GRAPHICS_DISPLAY
#include <vulkan/vulkan.h>
#endif

struct Vertex
{
	Vertex() {}
	Vertex(glm::vec3 _position) : position(_position) {}

	glm::vec3 position = glm::vec3(0, 0, 0);
	glm::vec3 normal = glm::vec3(0, 0, 1);
	glm::vec3 tangent = glm::vec3(1, 0, 0);
	glm::vec2 uv = glm::vec2(0.5f, 0.5f);

#if GRAPHICS_DISPLAY
	// about how to cut the binding-th array into strides
	static void getBindingDescription(VkVertexInputBindingDescription& bindingDescription)
	{
		bindingDescription = {
			.binding = 0, // binding index. Just one binding if all vertex data is passed as one array
			.stride = sizeof(Vertex),
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
			.offset = (uint32_t)offsetof(Vertex, position)
		});
		attributeDescriptions.push_back(VkVertexInputAttributeDescription{
			.location = 1,
			.binding = 0,
			.format = VK_FORMAT_R32G32B32_SFLOAT,
			.offset = (uint32_t)offsetof(Vertex, normal)
		});
		attributeDescriptions.push_back(VkVertexInputAttributeDescription{
			.location = 2,
			.binding = 0,
			.format = VK_FORMAT_R32G32B32_SFLOAT,
			.offset = (uint32_t)offsetof(Vertex, tangent)
		});
		attributeDescriptions.push_back(VkVertexInputAttributeDescription{
			.location = 3,
			.binding = 0,
			.format = VK_FORMAT_R32G32_SFLOAT,
			.offset = (uint32_t)offsetof(Vertex, uv)
		});
	}
#endif
};
static_assert(sizeof(Vertex) == sizeof(float) * (3 + 3 + 3 + 2), "vertex struct should be packed");
