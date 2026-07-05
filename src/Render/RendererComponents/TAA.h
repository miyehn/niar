#pragma once
#include "Render/Vulkan/DescriptorSet.h"

class Texture2D;

struct TAA
{
    TAA() = default;
    TAA(const TAA&) = delete;
    TAA& operator=(const TAA&) = delete;

    struct InitInfo {
        const Texture2D* sceneColor = nullptr;
        const Texture2D* GMotion = nullptr;
    };

    void init(const InitInfo& info);
    void release();

    // globalFrameIndex must be a counter that increments once per rendered frame (not the
    // frame-in-flight ring index), so the ping-pong toggle stays correct regardless of
    // MAX_FRAMES_IN_FLIGHT's parity.
    void render(VkCommandBuffer cmdbuf, uint32_t globalFrameIndex);
    void clear();
    void clear(VkCommandBuffer cmdbuf);

    const Texture2D* getResolved() const { return resolved; }

private:
    Texture2D* resolved = nullptr;
    Texture2D* history[2] = {};
    // indexed by history write slot (frameIndex % 2), mirroring GI's ping-pong convention
    DescriptorSet taaDescriptorSets[2];
};
