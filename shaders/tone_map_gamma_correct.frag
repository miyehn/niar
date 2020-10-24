#version 330 core

uniform sampler2D TEX;
uniform sampler2D BLOOM;
uniform int ToneMapping;
uniform int GammaCorrect;

in vec2 vf_uv;
out vec4 FragColor;

/* found a varietty of them here: https://www.shadertoy.com/view/WdjSW3
 * some more relevant links:
 * https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
 * http://seenaburns.com/dynamic-range/
 */

 // this one has "foot"
float Tonemap_ACES(float x) {
	// Narkowicz 2015, "ACES Filmic Tone Mapping Curve"
	const float a = 2.51;
	const float b = 0.03;
	const float c = 2.43;
	const float d = 0.59;
	const float e = 0.14;
	return (x * (a * x + b)) / (x * (c * x + d) + e);
}

// no "foot", only "shoulder"
float Reinhard2(float x) {
    const float L_white = 2.0;
    return (x * (1.0 + x / (L_white * L_white))) / (1.0 + x);
}

vec3 Tonemap_vec3(vec3 src) {
	return vec3(
		Reinhard2(src.r),
		Reinhard2(src.g),
		Reinhard2(src.b)
	);
}

void main() {

	// linear color
	vec3 linear = texture(TEX, vf_uv).rgb + texture(BLOOM, vf_uv).rgb;

	// exposure adjustment: https://stackoverflow.com/questions/12166117/what-is-the-math-behind-exposure-adjustment-on-photoshop
	// vec3 exposed = linear * pow(2, Exposure);

	// tone mapping (see above links)
	vec3 toneMapped = ToneMapping>0 ? Tonemap_vec3(linear) : linear;

	// gamma correction
	const vec3 gamma = vec3(0.455);
	vec3 gammaCorrected = GammaCorrect>0 ? pow(toneMapped, gamma) : toneMapped;

	// output
	FragColor = vec4( gammaCorrected, 1);

}
