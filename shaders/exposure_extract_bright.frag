#version 330 core

uniform sampler2D TEX;
uniform float Exposure;
uniform float BloomThreshold;

in vec2 vf_uv;
layout(location=0) out vec4 Exposed;
layout(location=1) out vec4 BrightColors;

// took from Unity
float Luminance(vec3 col) {
	return dot(col, vec3(0.22, 0.707, 0.071));
}

void main() {

	// linear color
	vec3 linear = texture(TEX, vf_uv).rgb;

	// exposure adjustment: https://stackoverflow.com/questions/12166117/what-is-the-math-behind-exposure-adjustment-on-photoshop
	vec3 exposed = linear * pow(2, Exposure);

	// output
	Exposed = vec4(exposed, 1);
	BrightColors = max(vec4(0), Exposed - vec4(BloomThreshold));
	// BrightColors = Luminance(exposed) >= BloomThreshold ? Exposed : vec4(0);
}
