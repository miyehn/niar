#version 330 core

uniform sampler2D GBUF0;
uniform sampler2D GBUF1;
uniform sampler2D GBUF2;

in vec2 vf_uv;
out vec4 FragColor;

void main() {

	vec4 GPosition = texture(GBUF0, vf_uv);
	vec4 GNormal = texture(GBUF1, vf_uv);
	vec4 GColor = texture(GBUF2, vf_uv);
  FragColor = GPosition * 0 + GNormal * 0 + GColor * 1;

}

