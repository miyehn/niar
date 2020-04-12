#version 330 core

uniform mat4 OBJECT_TO_CLIP;
uniform mat3 OBJECT_TO_CAM_ROT;

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec4 in_color;

out vec3 vf_normal;
out vec4 vf_color;

void main() {
  gl_Position = OBJECT_TO_CLIP * vec4(in_position, 1);
  vf_normal = OBJECT_TO_CAM_ROT * in_normal;
  vf_color = in_color;
}
