#version 330 core

uniform sampler2D TEX;
uniform float InvWidth;

in vec2 vf_uv;
out vec4 FragColor;

// gaussian kernel calculator: http://dev.theomader.com/gaussian-kernel-calculator/

#define TAIL_LEN 10
#define CENTER_WEIGHT 0.056461
uniform float weights[TAIL_LEN] = float[](0.05618, 0.055344, 0.053979, 0.052124, 0.049832, 0.047168, 0.044202, 0.04101, 0.037671, 0.03426);

void main() {

	FragColor = texture(TEX, vf_uv) * CENTER_WEIGHT;
	for (int i=0; i<TAIL_LEN; i++)
	{
		vec2 uv = vf_uv; uv.x += InvWidth * i;
		FragColor += texture(TEX, uv) * weights[i];

		uv = vf_uv; uv.x -= InvWidth * i;
		FragColor += texture(TEX, uv) * weights[i];
	}

}