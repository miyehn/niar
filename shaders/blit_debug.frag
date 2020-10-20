#version 330 core

uniform sampler2D TEX;
uniform vec2 MinMax;

in vec2 vf_uv;
out vec4 FragColor;

void main() {

	vec4 tex = max(min(texture(TEX, vf_uv), MinMax.y), MinMax.x);
	tex = mix(vec4(0.), vec4(1.), (tex - MinMax.x) / (MinMax.y - MinMax.x));
 	FragColor = tex;

}
