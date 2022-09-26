layout(location=0) in vec4 vf_position;
layout(location=1) in vec2 vf_uv;
layout(location=2) in mat3 TANGENT_TO_WORLD_ROT;

layout(set = 3, binding = 1) uniform MaterialParamsBufferObject {
    vec4 BaseColorFactor;
    vec4 OcclusionRoughnessMetallicNormalStrengths;
    vec4 EmissiveFactorClipThreshold;
    vec4 _pad0;
} materialParams;

layout(set = 3, binding = 2) uniform sampler2D AlbedoMap;
layout(set = 3, binding = 3) uniform sampler2D NormalMap;
layout(set = 3, binding = 4) uniform sampler2D ORMMap;
layout(set = 3, binding = 5) uniform sampler2D EmissiveMap;
