#version 330 core

uniform mat3 OBJECT_TO_WORLD_ROT;
uniform mat4 OBJECT_TO_WORLD;
uniform mat4 OBJECT_TO_CLIP;

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec4 in_color;

out vec4 vf_position;
out vec3 vf_normal;
out vec3 vf_color;

out float Z;

void main() {
	gl_Position = OBJECT_TO_CLIP * vec4(in_position, 1);
  vf_position = OBJECT_TO_WORLD * vec4(in_position, 1);

	vec3 normal = OBJECT_TO_WORLD_ROT * in_normal;
	vec3 color = in_color.rgb;

	// perspective-correct lerp: http://15462.courses.cs.cmu.edu/spring2019/lecture/texture/slide_033
	float z = gl_Position.z / gl_Position.w;
  vf_normal = normal / z;
  vf_color = color / z;
	Z = 1.0f / z;
}
