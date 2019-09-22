#version 430 core
#define PI 3.1415926535897932384626433832795

layout (quads, ccw) in;
in vec4 c_data[];
uniform mat4 transformation;
out vec3 normal;

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

float norm_orientation = orientation + PI/2;
vec3 align_n = vec3(cos(orientation), sin(orientation), 0);
vec3 n0 = vec3(cos(norm_orientation), sin(norm_orientation), 0); // TODO: optimize this
vec3 n1 = cross( ctrl / length(ctrl), align_n );
vec3 above_2_ctrl = ctrl - above;
vec3 n2 = cross( above_2_ctrl / length(above_2_ctrl), align_n );

void main() {
  gl_Position = transformation * vec4(
     spine.x+cos_halfWidth, spine.y+sin_halfWidth, spine.z,
  1);
  vec3 n = n0 * (1-t)*(1-t) + n1 * 2*(1-t)*t + n2 * t*t;
  normal = n / length(n);
}
