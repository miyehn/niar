#version 330 core

in vec3 vf_normal;

out vec4 FragColor;

void main() {
  vec3 light_dir = normalize(vec3(0, 0.5, 1));
	float brightness = clamp(dot(normalize(vf_normal), light_dir), 0, 1);

  FragColor = vec4(mix(0.4, 1, brightness));
}
