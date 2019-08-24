#version 410 core

layout (vertices = 4) out;
in float v_gray[]; // length of input patch size

out float c_gray[]; // length of output patch size

void main() {
  vec4 inPos = gl_in[0].gl_Position;
  // assign each out vertex some position?
  if (gl_InvocationID == 0) {
    gl_TessLevelInner[0] = 2.0;
    gl_TessLevelInner[1] = 2.0;
    gl_TessLevelOuter[0] = 2.0;
    gl_TessLevelOuter[1] = 2.0;
    gl_TessLevelOuter[2] = 2.0;
    gl_TessLevelOuter[3] = 2.0;
    gl_out[gl_InvocationID].gl_Position = inPos;
  } else if (gl_InvocationID == 1) {
    gl_out[gl_InvocationID].gl_Position = vec4(inPos.x + 0.1, inPos.y, inPos.z, inPos.w);
  } else if (gl_InvocationID == 2) {
    gl_out[gl_InvocationID].gl_Position = vec4(inPos.x + 0.1, inPos.y + 0.1, inPos.z, inPos.w);
  } else {
    gl_out[gl_InvocationID].gl_Position = vec4(inPos.x, inPos.y + 0.1, inPos.z, inPos.w);
  }

  c_gray[gl_InvocationID] = v_gray[0];
}
