#version 450 core

layout(location=0) in vec4 vf_position;
layout(location=1) in vec2 vf_uv;
layout(location=2) in mat3 TANGENT_TO_WORLD_ROT;

layout(set = 0, binding = 0) uniform UniformBufferObject {
	mat4 ModelMatrix;
	mat4 ViewMatrix;
	mat4 ProjectionMatrix;
	vec3 Tint;
} ubo;

layout(set = 0, binding = 1) uniform sampler2D AlbedoMap;
layout(set = 0, binding = 2) uniform sampler2D NormalMap;
layout(set = 0, binding = 3) uniform sampler2D MetallicMap;
layout(set = 0, binding = 4) uniform sampler2D RoughnessMap;
layout(set = 0, binding = 5) uniform sampler2D AOMap;

layout(location=0) out vec3 Position;
layout(location=1) out vec3 Normal;
layout(location=2) out vec3 Color;
layout(location=3) out vec3 MRA;

void main()
{
	vec2 uv = vf_uv;

	Position = vf_position.xyz;
	vec3 sampled_normal = texture(NormalMap, uv).rgb * 2 - 1.0;
	sampled_normal.rg = -sampled_normal.rg;
	Normal = TANGENT_TO_WORLD_ROT * normalize(sampled_normal);
	Color = texture(AlbedoMap, uv).rgb * ubo.Tint;

	float metallic = texture(MetallicMap, uv).r;
	float roughness = texture(RoughnessMap, uv).r;
	float ao = texture(AOMap, uv).r;
	MRA = vec3(metallic, roughness, ao);
}