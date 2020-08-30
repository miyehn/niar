#version 330 core

in vec3 vf_normal;
in vec2 vf_uv;

uniform sampler2D BaseColor;
uniform vec3 Color;

out vec4 FragColor;

void main() {
  vec3 light_dir = normalize(vec3(0, 0.5, 1));
	float brightness = clamp(dot(normalize(vf_normal), light_dir), 0, 1);

	vec3 color = texture(BaseColor, vf_uv).rgb * Color;

  FragColor = vec4(color * mix(0.4, 1, brightness), 1);
}
