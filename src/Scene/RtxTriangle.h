#pragma once

#include "Engine/SceneObject.hpp"
#include "Render/Vulkan/Buffer.h"
#include "Render/Vulkan/DescriptorSet.h"

class Texture2D;

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

	void create_sbt();

	VmaBuffer blasBuffer;
	VkAccelerationStructureKHR blas;

	VmaBuffer tlasBuffer;
	VkAccelerationStructureKHR tlas;

	DescriptorSet descriptorSet;

	Texture2D* outImage;

	// singletons (TODO: clean up later)
	VkPipeline pipeline;
	VkPipelineLayout pipelineLayout;
};

