#version 430 core

layout (vertices = 4) out;

in vec4 v_data[];
out vec4 c_data[];

void main() {
  c_data[gl_InvocationID] = v_data[gl_InvocationID];

  if (gl_InvocationID == 0) {
    gl_TessLevelInner[0] = 1.0;
    gl_TessLevelInner[1] = 4.0;
    gl_TessLevelOuter[0] = 4.0;
    gl_TessLevelOuter[1] = 1.0;
    gl_TessLevelOuter[2] = 4.0;
    gl_TessLevelOuter[3] = 1.0;
  }
}
