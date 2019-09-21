#version 430 core

layout (quads, ccw) in;
in vec4 c_data[];
uniform mat4 transformation;
out vec4 col;

// unpack data from input
vec3 root = c_data[0].xyz;
vec3 above = c_data[1].xyz;
vec3 ctrl = c_data[2].xyz;
vec3 up = c_data[3].xyz; 
float width = c_data[0].w;
float height = c_data[1].w;
float stiffness = c_data[2].w;
float orientation = c_data[3].w;
float halfWidth_root = (gl_TessCoord.x - 0.5) * width;
float t = gl_TessCoord.y;

vec3 spine = root + 2*(1-t)*t*above + t*t*ctrl;
float halfWidth = (1 - 0.8*t) * halfWidth_root;
float cos_halfWidth = halfWidth * cos(orientation);
float sin_halfWidth = halfWidth * sin(orientation);

void main() {
  gl_Position = transformation * vec4(
     spine.x+cos_halfWidth, spine.y+sin_halfWidth, spine.z,
  1);
  col = vec4(0.5231, 0.6361, 0.253, 0.92);
}
