#version 450 core

#include "scene_common.glsl"

layout(set = 3, binding = 0) uniform sampler2D SceneColor;
layout(set = 3, binding = 1) uniform sampler2D SceneDepth;

layout(location = 0) in vec2 vf_uv;
layout(location = 0) out vec4 FragColor;

/* found a varietty of them here: https://www.shadertoy.com/view/WdjSW3
 * some more relevant links:
 * https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
 * http://seenaburns.com/dynamic-range/
 */

// from: https://github.com/SaschaWillems/Vulkan-glTF-PBR/blob/master/data/shaders/pbr_khr.frag
vec3 SRGBtoLINEAR(vec3 srgbIn)
{
    #ifdef SRGB_FAST_APPROXIMATION
    vec3 linOut = pow(srgbIn.xyz,vec3(2.2));
    #else //SRGB_FAST_APPROXIMATION
    vec3 bLess = step(vec3(0.04045),srgbIn.xyz);
    vec3 linOut = mix( srgbIn.xyz/vec3(12.92), pow((srgbIn.xyz+vec3(0.055))/vec3(1.055),vec3(2.4)), bLess );
    #endif //SRGB_FAST_APPROXIMATION
    return linOut;
}

// this one has "foot"
float Tonemap_ACES(float x) {
    // Narkowicz 2015, "ACES Filmic Tone Mapping Curve"
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return (x * (a * x + b)) / (x * (c * x + d) + e);
}

// no "foot", only "shoulder"
float Reinhard2(float x) {
    const float L_white = 2.0;
    return (x * (1.0 + x / (L_white * L_white))) / (1.0 + x);
}

vec3 Reinhard_vec3(vec3 src) {
    return vec3(
    Reinhard2(src.r),
    Reinhard2(src.g),
    Reinhard2(src.b)
    );
}

vec3 ACES_vec3(vec3 src)
{
    return vec3(
    Tonemap_ACES(src.r),
    Tonemap_ACES(src.g),
    Tonemap_ACES(src.b)
    );
}

void main()
{
    FragColor = vec4(0, 0, 0, 1);
    ViewInfo viewInfo = GetViewInfo();

    // linear color
    vec3 linear = texture(SceneColor, vf_uv).rgb;

    FragColor.rgb = linear;

    // exposure adjustment: https://stackoverflow.com/questions/12166117/what-is-the-math-behind-exposure-adjustment-on-photoshop
    FragColor.rgb = linear * pow(2, viewInfo.Exposure);

    // tone mapping
    if (viewInfo.ToneMappingOption == 1)
    {
        FragColor.rgb = Reinhard_vec3(FragColor.rgb);
    }
    else if (viewInfo.ToneMappingOption == 2)
    {
        FragColor.rgb = ACES_vec3(FragColor.rgb);
    }
}