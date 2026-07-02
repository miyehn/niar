#ifndef _GLTF_BINDLESS_MATERIAL
#define _GLTF_BINDLESS_MATERIAL

#include "bindless_resources.glsl"

const int GLTF_MATERIAL_TEXTURE_ALBEDO = 0;
const int GLTF_MATERIAL_TEXTURE_NORMAL = 1;
const int GLTF_MATERIAL_TEXTURE_ORM = 2;
const int GLTF_MATERIAL_TEXTURE_EMISSIVE = 3;

GpuMaterial getGltfMaterial(uint materialIndex) {
    return BindlessMaterials[materialIndex];
}

vec4 sampleGltfMaterialTexture(GpuMaterial material, int textureSlot, vec2 uv) {
    const uint bindlessTextureIndex = material.textureIndices[textureSlot];
    return sampleBindlessTexture2D(bindlessTextureIndex, uv);
}

#endif
