#version 330 core

uniform mat4 OBJECT_TO_CLIP;//Camera::Active->world_to_clip() * cube->object_to_world();
uniform mat3 OBJECT_TO_CAM_ROT;//obj->object_to_world_rotation() * Camera::Active->world_to_camera_rotation();

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec3 in_tangent;
layout (location = 3) in vec2 in_uv;

out vec3 vf_normal;
out vec2 vf_uv;

void main() {
  gl_Position = OBJECT_TO_CLIP * vec4(in_position, 1);
  vf_normal = OBJECT_TO_CAM_ROT * in_normal;
	vf_uv = in_uv;
}
