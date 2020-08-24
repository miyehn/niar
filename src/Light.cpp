#include "Light.hpp"
#include "Camera.hpp"
#include "Globals.hpp"
#include "Scene.hpp"
#include "Program.hpp"

DirectionalLight::DirectionalLight(vec3 _color, float _intensity, vec3 dir) {

	type = Directional;
	
	color = _color;
	intensity = _intensity;
	shadow_map_dim = 1024;
	set_direction(dir);

	effective_radius = 10.0f;

	shadow_map_cam = new Camera(effective_radius*2, effective_radius*2, true, false);
	shadow_map_cam->lock();
	shadow_map_cam->cutoffNear = 1.0f;

	//------- buffer generations -------

	// shadow map (framebuffer)
	glGenFramebuffers(1, &shadow_map_fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, shadow_map_fbo);
	{
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
			ERR("directional light shadow map framebuffer not correctly initialized");
	}
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
	// TODO: make this change dynamically based on scene content
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

PointLight::PointLight(vec3 _color, float _intensity, vec3 _local_pos) {
	
	type = Point;

	color = _color;
	intensity = _intensity;
	local_position = _local_pos;
	shadow_map_dim = 512;

	effective_radius = 16.0f; // where intensity 1 attenuates to 1/255

	shadow_map_cam = new Camera(effective_radius*2, effective_radius*2, false, false);
	shadow_map_cam->lock();
	shadow_map_cam->cutoffNear = 0.5f;
	shadow_map_cam->fov = radians(90.0f);
	shadow_map_cam->aspect_ratio = 1.0f;

	//------- buffer generations -------

	// shadow map framebuffers
	glGenFramebuffers(6, shadow_map_fbos);

	// generate cube map and bind each face to an fbo (to be rendered to later)
	glGenTextures(1, &shadow_map_tex); 
	glBindTexture(GL_TEXTURE_CUBE_MAP, shadow_map_tex);

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	for (int i=0; i<6; i++) {
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT, 
				shadow_map_dim, shadow_map_dim, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
		// set a face of cube map to an fbo
		glBindFramebuffer(GL_FRAMEBUFFER, shadow_map_fbos[i]);
		glFramebufferTexture2D(
				GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_CUBE_MAP_POSITIVE_X+i, shadow_map_tex, 0);
		glDrawBuffer(GL_NONE);
		glReadBuffer(GL_NONE);
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE)
			ERR("point light shadow map framebuffer not correctly initialized");
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

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
	new NamedTex("point light shadow mask", shadow_mask_tex);
}

void PointLight::render_shadow_map() {

	effective_radius = sqrt(255.0f * intensity); // a heuristic

	shadow_map_cam->position = world_position();
	shadow_map_cam->cutoffFar = effective_radius;

	Camera* cached_camera = Camera::Active;
	Camera::Active = shadow_map_cam;

	Scene* scene = get_scene();
	scene->shader_set = 1;
	glViewport(0, 0, shadow_map_dim, shadow_map_dim);

	// TODO: render these 6 faces
	for (int i=0; i<6; i++) {
		//if (i==4) continue;
		// TODO: use glm::lookAt here
		// see: https://learnopengl.com/Advanced-Lighting/Shadows/Point-Shadows
		// also make sure orientation is correct: https://www.khronos.org/opengl/wiki/Cubemap_Texture#Upload_and_orientation
		//shadow_map_cam->rotation = quat_from_dir(shadow_map_normals[i]);
		vec3 up;
		vec3 dir;
		if (i==2) up = vec3(0, 0, -1);
		else if (i==3) up = vec3(0, 0, 1);
		else up = vec3(0, -1, 0);

		if (i==2 || i==3) dir = -shadow_map_normals[i];
		else dir = shadow_map_normals[i];

		shadow_map_cam->rotation = lookAt(
				shadow_map_cam->position, 
				shadow_map_cam->position + dir,
				up);

		glBindFramebuffer(GL_FRAMEBUFFER, shadow_map_fbos[i]);
		glClear(GL_DEPTH_BUFFER_BIT);
		scene->draw_content(true);
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	Camera::Active = cached_camera;
}
