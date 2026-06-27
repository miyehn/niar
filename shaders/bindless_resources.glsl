#ifndef _BINDLESS_RESOURCES
#define _BINDLESS_RESOURCES

#extension GL_EXT_nonuniform_qualifier : require

#include "cshared/cshared_common.glsl"

layout(set = DSET_BINDLESS, binding = 0)
uniform sampler2D BindlessTextures[MAX_BINDLESS_TEXTURES_2D];

layout(set = DSET_BINDLESS, binding = 1, std430)
readonly buffer BindlessMaterialTable {
    GpuMaterial BindlessMaterials[];
};

vec4 sampleBindlessTexture2D(uint index, vec2 uv) {
    return texture(BindlessTextures[nonuniformEXT(index)], uv);
}

vec4 sampleBindlessTexture2DLod(uint index, vec2 uv, float lod) {
    return textureLod(BindlessTextures[nonuniformEXT(index)], uv, lod);
}

#endif
