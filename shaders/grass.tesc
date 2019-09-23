#version 430 core

layout (vertices = 4) out;
uniform mat4 transformation;

in vec4 v_data[];
out vec4 c_data[];

vec3 cam_space(vec4 v) {
  vec4 v_cam_space = transformation * v;
  return v_cam_space.xyz / v_cam_space.w;
}

bool outside_frustrum(vec4 v) {
  float offset = 0.2;
  vec3 v_cam_space = cam_space(v);
  bool out_horizontal = v_cam_space.x < -1-offset || v_cam_space.x > 1+offset;
  bool out_vertical = v_cam_space.y < -1-offset || v_cam_space.y > 1+offset;
  return out_horizontal || out_vertical;
}

float map(float min1, float max1, float min2, float max2, float v) {
  return min2 + (max2-min2) * (v-min1) / (max1-min1);
}

void main() {
  c_data[gl_InvocationID] = v_data[gl_InvocationID];

  if (gl_InvocationID == 1) {
    vec3 root = v_data[0].xyz;

    vec4 root4 = vec4(root, 1);
    vec4 rt_plus_above = root4 + vec4(v_data[1].xyz, 0);
    vec4 rt_plus_ctrl = root4 + vec4(v_data[2].xyz, 0);

    // culling test
    bool cull = outside_frustrum(root4) &&
      outside_frustrum(rt_plus_above) &&
      outside_frustrum(rt_plus_ctrl);

    // decide LOD based on how far from camera
    // depth seems to have a range that looks like 0.975 to 0.9999 ???
    float depth = cam_space(root4).z;
    float lod = max(1.0, floor(map(0.99, 1, 5.5, 0.5, depth)));

    gl_TessLevelInner[0] = 1.0;
    gl_TessLevelInner[1] = lod;
    gl_TessLevelOuter[0] = lod;
    gl_TessLevelOuter[1] = cull ? -1.0 : 1.0;
    gl_TessLevelOuter[2] = lod;
    gl_TessLevelOuter[3] = 1.0;
  }
}
