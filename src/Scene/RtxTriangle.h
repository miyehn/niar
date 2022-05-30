#pragma once

#include "Engine/SceneObject.hpp"
#include "Render/Vulkan/Buffer.h"
#include "Render/Vulkan/DescriptorSet.h"

class Texture2D;
class ShaderBindingTable;

class RtxTriangle : public SceneObject
{
public:
	RtxTriangle();
	~RtxTriangle() override;
	void update(float elapsed) override;

	//---- vulkan stuff ----

	VmaBuffer vertexBuffer;
	VmaBuffer indexBuffer;

	void create_vertex_buffer();
	void create_index_buffer();

	VmaBuffer blasBuffer;
	VkAccelerationStructureKHR blas;

	VmaBuffer tlasBuffer;
	VkAccelerationStructureKHR tlas;

	DescriptorSet descriptorSet;

	Texture2D* outImage;

	ShaderBindingTable* sbt;

	// singletons (TODO: clean up later)
	VkPipeline pipeline;
	VkPipelineLayout pipelineLayout;
};

