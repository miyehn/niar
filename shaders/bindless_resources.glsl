#ifndef _BINDLESS_RESOURCES
#define _BINDLESS_RESOURCES

#extension GL_EXT_nonuniform_qualifier : require

layout(set = DSET_BINDLESS, binding = 0)
uniform sampler2D BindlessTextures[MAX_BINDLESS_TEXTURES_2D];

vec4 sampleBindlessTexture2D(uint index, vec2 uv)
{
    return texture(BindlessTextures[nonuniformEXT(index)], uv);
}

vec4 sampleBindlessTexture2DLod(uint index, vec2 uv, float lod)
{
    return textureLod(BindlessTextures[nonuniformEXT(index)], uv, lod);
}

#endif
