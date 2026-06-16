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

    struct InitInfo {
        const Texture2D* GPosition = nullptr;
        const Texture2D* GNormal = nullptr;
        const VmaBuffer* viewInfoUbos[MAX_FRAMES_IN_FLIGHT] = {};
        VkAccelerationStructureKHR tlas = VK_NULL_HANDLE;
        const Texture2D* environmentMap = nullptr;
    };

    void init(const InitInfo& info);
    void release();

    void render(VkCommandBuffer cmdbuf, uint32_t frameIndex, const DescriptorSet& skyDescriptorSet);
    void clear();
    void clear(VkCommandBuffer cmdbuf);

    const Texture2D* getIndirectLighting() const { return indirectLighting; }

private:
    bool enabledLastFrame = false;
    Texture2D* indirectLighting = nullptr;
    DescriptorSet giDescriptorSets[MAX_FRAMES_IN_FLIGHT];

};

