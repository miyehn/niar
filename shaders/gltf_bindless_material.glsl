#ifndef _GLTF_BINDLESS_MATERIAL
#define _GLTF_BINDLESS_MATERIAL

#include "bindless_resources.glsl"

layout(location=0) in vec4 vf_position;
layout(location=1) in vec2 vf_uv;
layout(location=2) in mat3 TANGENT_TO_WORLD_ROT;

layout(push_constant) uniform GltfFragmentPushConstants {
    layout(offset = GLTF_MATERIAL_INDEX_PUSH_OFFSET) uint MaterialIndex;
} pc;

const int GLTF_MATERIAL_TEXTURE_ALBEDO = 0;
const int GLTF_MATERIAL_TEXTURE_NORMAL = 1;
const int GLTF_MATERIAL_TEXTURE_ORM = 2;
const int GLTF_MATERIAL_TEXTURE_EMISSIVE = 3;

GpuMaterial getGltfMaterial() {
    return BindlessMaterials[pc.MaterialIndex];
}

vec4 sampleGltfMaterialTexture(GpuMaterial material, int textureSlot, vec2 uv) {
    const uint bindlessTextureIndex = material.textureIndices[textureSlot];
    return sampleBindlessTexture2D(bindlessTextureIndex, uv);
}

#endif
