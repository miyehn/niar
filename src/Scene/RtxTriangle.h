#pragma once

#include "Engine/SceneObject.hpp"
#include "Render/Vulkan/Buffer.h"

class RtxTriangle : public SceneObject
{
public:
	RtxTriangle();
	~RtxTriangle();
	void draw(VkCommandBuffer cmdbuf) override;

	//---- vulkan stuff ----

	VmaBuffer vertexBuffer;
	VmaBuffer indexBuffer;

	void create_vertex_buffer();
	void create_index_buffer();
};

