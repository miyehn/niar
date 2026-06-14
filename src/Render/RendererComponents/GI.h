//
// Created by miyehn on 6/11/2026.
//
#pragma once
#include "Render/Vulkan/Vulkan.hpp"
#include "Render/Vulkan/DescriptorSet.h"

class Texture2D;
class VmaBuffer;

struct GI
{
    GI() = default;
    GI(const GI&) = delete;
    GI& operator=(const GI&) = delete;

	// Milestone 1 bridge: compute a standalone indirectLighting texture, then add it to sceneColor.

    struct InitInfo {
        const Texture2D* GPosition = nullptr;
        const Texture2D* GNormal = nullptr;
        const Texture2D* sceneColor = nullptr;
        const VmaBuffer* viewInfoUbos[MAX_FRAMES_IN_FLIGHT] = {};
        VkAccelerationStructureKHR tlas = VK_NULL_HANDLE;
    };

    void init(const InitInfo& info);
    void release();

    void render(VkCommandBuffer cmdbuf, uint32_t frameIndex, const DescriptorSet& skyDescriptorSet);

private:
    const Texture2D* sceneColor = nullptr; // storing a ptr so it can add barriers in render(...)
    Texture2D* indirectLighting = nullptr;
    DescriptorSet giDescriptorSets[MAX_FRAMES_IN_FLIGHT];

};

