#include "Light.hpp"
#include "Camera.hpp"
#include "Globals.hpp"

DirectionalLight::DirectionalLight(vec3 _color, float _intensity, vec3 dir) : 
		color(_color), intensity(_intensity) {

	set_direction(dir);

	shadow_map_cam = new Camera(effective_radius*2, effective_radius*2, true, false);
	shadow_map_cam->cutoffNear = 1.0f;

	glGenFramebuffers(1, &shadow_map_fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, shadow_map_fbo);

	glGenTextures(1, &shadow_map_tex);
	glBindTexture(GL_TEXTURE_2D, shadow_map_tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, shadow_map_dim, shadow_map_dim, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadow_map_tex, 0);

	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE)
		ERR("framebuffer not correctly initialized");
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	new NamedTex("directional map", shadow_map_tex);
}

DirectionalLight::~DirectionalLight() {
	delete shadow_map_cam;
}

void DirectionalLight::render_shadow_map() {
	shadow_map_cam->rotation = rotation;
	shadow_map_cam->position = world_position() - rotation * vec3(0, 0, -1) * (effective_radius + 1);
	shadow_map_cam->cutoffFar = 2.0f + effective_radius * 2;
}

void PointLight::render_shadow_map() {
}
