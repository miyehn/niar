#pragma once

#include "Engine/SceneObject.hpp"

class RtxTriangle : public SceneObject
{
public:
	RtxTriangle() { name = "RTX Triangle"; }
	void draw(VkCommandBuffer cmdbuf) override;
};

