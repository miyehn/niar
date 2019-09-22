#version 430 core

layout (vertices = 4) out;
uniform mat4 transformation;

in vec4 v_data[];
out vec4 c_data[];

bool outside_frustrum(vec4 v) {
  float offset = 0.2;
  vec4 v_cam_space = transformation * v;
  vec3 v_cam_space_3 = v_cam_space.xyz / v_cam_space.w;
  bool out_horizontal = v_cam_space_3.x < -1-offset || v_cam_space_3.x > 1+offset;
  bool out_vertical = v_cam_space_3.y < -1-offset || v_cam_space_3.y > 1+offset;
  return out_horizontal || out_vertical;
}

void main() {
  c_data[gl_InvocationID] = v_data[gl_InvocationID];

  if (gl_InvocationID == 0) {

    vec4 root = vec4(v_data[0].xyz, 1);
    vec4 rt_plus_above = root + vec4(v_data[1].xyz, 0);
    vec4 rt_plus_ctrl = root + vec4(v_data[2].xyz, 0);

    bool cull = outside_frustrum(root) &&
      outside_frustrum(rt_plus_above) &&
      outside_frustrum(rt_plus_ctrl);

    gl_TessLevelInner[0] = 1.0;
    gl_TessLevelInner[1] = 4.0;
    gl_TessLevelOuter[0] = cull ? -1 : 4.0;
    gl_TessLevelOuter[1] = 1.0;
    gl_TessLevelOuter[2] = 4.0;
    gl_TessLevelOuter[3] = 1.0;
  }
}
