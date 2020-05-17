#include "Light.hpp"
#include "Camera.hpp"
#include "Globals.hpp"
#include "Scene.hpp"
#include "Program.hpp"

DirectionalLight::DirectionalLight(vec3 _color, float _intensity, vec3 dir) : 
		color(_color), intensity(_intensity) {

	set_direction(dir);

	shadow_map_cam = new Camera(effective_radius*2, effective_radius*2, true, false);
	shadow_map_cam->lock();
	shadow_map_cam->cutoffNear = 1.0f;

	effective_radius = 10.0f;

	//------- buffer generations -------

	// shadow map (framebuffer)
	glGenFramebuffers(1, &shadow_map_fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, shadow_map_fbo);

	glGenTextures(1, &shadow_map_tex);
	glBindTexture(GL_TEXTURE_2D, shadow_map_tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, shadow_map_dim, shadow_map_dim, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	float border_color[] = {1.0f, 1.0f, 1.0f, 1.0f};
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border_color);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadow_map_tex, 0);

	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE)
		ERR("shadow map framebuffer not correctly initialized");
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	new NamedTex("directional light shadow map", shadow_map_tex);

	// shadow mask (output)
	int w, h;
	SDL_GL_GetDrawableSize(Program::Instance->window, &w, &h);

	glGenTextures(1, &shadow_mask_tex);
	glBindTexture(GL_TEXTURE_2D, shadow_mask_tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, w, h, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);
	new NamedTex("directional light shadow mask", shadow_mask_tex);
}

DirectionalLight::~DirectionalLight() {
	delete shadow_map_cam;
}

void DirectionalLight::render_shadow_map() {
	
	// set camera properties
	shadow_map_cam->position = world_position() - rotation * vec3(0, 0, -1) * (effective_radius + 1);
	shadow_map_cam->rotation = rotation;

	shadow_map_cam->width = effective_radius * 2;
	shadow_map_cam->height = effective_radius * 2;

	shadow_map_cam->cutoffFar = 2.0f + effective_radius * 2;

	// prepare to draw the scene
	Camera* cached_camera = Camera::Active;
	Camera::Active = shadow_map_cam;

	glBindFramebuffer(GL_FRAMEBUFFER, shadow_map_fbo);
	glClear(GL_DEPTH_BUFFER_BIT);

	Scene* scene = get_scene();
	scene->shader_set = 1;
	glViewport(0, 0, shadow_map_dim, shadow_map_dim);
	scene->draw_content(true);

	Camera::Active = cached_camera;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

mat4 DirectionalLight::world_to_light_clip() {
	return shadow_map_cam->world_to_clip();
}

void PointLight::render_shadow_map() {
}
