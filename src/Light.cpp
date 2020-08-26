#include "Light.hpp"
#include "Camera.hpp"
#include "Globals.hpp"
#include "Scene.hpp"
#include "Program.hpp"
#include "Mesh.hpp"

// TODO
Light::~Light() {
	delete shadow_map_cam;
}

void DirectionalLight::init(vec3 _color, float _intensity, vec3 dir) {
	type = Directional;
	
	color = _color;
	intensity = _intensity;
	set_direction(dir);

	effective_radius = 10.0f;

	glGenTextures(1, &shadow_mask_tex);
}

DirectionalLight::DirectionalLight(vec3 _color, float _intensity, vec3 dir) {
	init(_color, _intensity, dir);
	name = "[unnamed directional light]";
}

DirectionalLight::DirectionalLight(aiLight* light, aiNode* mRootNode) {
	if (!mRootNode) ERR("trying to create light without giving root node");

	const char* inName = light->mName.C_Str();
	aiColor3D col = light->mColorDiffuse;
	aiVector3D dir = light->mDirection;

	// transformation (direction)
	aiNode* node = mRootNode->FindNode(inName);
	for (int i=0; i<3; i++) {
		if (node==nullptr) {
			ERR("directional light import: has less than 3 parent (rotation) nodes");
			break;
		}
		dir = node->mTransformation * dir;
		node = node->mParent;
	}
	vec3 dir_gl = normalize(vec3(dir.x, dir.y, dir.z));

	init(vec3(col.r, col.g, col.b), 1.0f, dir_gl);
	name = inName;
}

// TODO
DirectionalLight::~DirectionalLight() {
}

void DirectionalLight::render_shadow_map() {

	if (!cast_shadow) return;
	
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

void DirectionalLight::set_cast_shadow(bool cast) {
	if (!cast) {
		cast_shadow = false;
		return;
	}
	if (shadow_map_initialized) {
		cast_shadow = true;
		return;
	}

	// cast == true; shadow map needs initialization
	cast_shadow = true;
	shadow_map_initialized = true;

	shadow_map_dim = 1024;

	shadow_map_cam = new Camera(effective_radius*2, effective_radius*2, true, false);
	shadow_map_cam->lock();
	shadow_map_cam->cutoffNear = 1.0f;
	
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

	// shadow mask (output)
	int w = Program::Instance->drawable_width;
	int h = Program::Instance->drawable_height;

	glBindTexture(GL_TEXTURE_2D, shadow_mask_tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, w, h, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);
	new NamedTex(name + " shadow mask", shadow_mask_tex);
}

//-------- point light --------

void PointLight::init(vec3 _color, float _intensity, vec3 _local_pos) {
	LOGF("pos: %f %f %f", _local_pos.x, _local_pos.y, _local_pos.z);

	type = Point;

	color = _color;
	intensity = _intensity;
	local_position = _local_pos;

	glGenTextures(1, &shadow_mask_tex);
}

PointLight::PointLight(vec3 _color, float _intensity, vec3 _local_pos) {
	init(_color, _intensity, _local_pos);
	name = "[unnamed point light]";
}

PointLight::PointLight(aiLight* light, aiNode* mRootNode) {
	if (!mRootNode) ERR("trying to create light without giving root node");

	const char* inName = light->mName.C_Str();
	aiColor3D col = light->mColorDiffuse;
	aiVector3D pos = aiVector3D();

	// transformation (position)
	aiNode* node = mRootNode->FindNode(inName);
	while (node != nullptr) {
		pos = node->mTransformation * pos;
		node = node->mParent;
	}
	init(vec3(col.r, col.g, col.b), 1.0f, vec3(pos.x, pos.y, pos.z));
	name = inName;
}

// TODO
PointLight::~PointLight() {
}

void PointLight::render_shadow_map() {

	if (!cast_shadow) return;

	effective_radius = sqrt(255.0f * intensity); // a heuristic

	shadow_map_cam->position = world_position();
	shadow_map_cam->cutoffFar = effective_radius;

	Camera* cached_camera = Camera::Active;
	Camera::Active = shadow_map_cam;

	Scene* scene = get_scene();
	scene->shader_set = 5;
	set_location_to_all_shaders(scene, 5);
	glViewport(0, 0, shadow_map_dim, shadow_map_dim);

	// TODO: merge these 6 calls into one (MRT)
	for (int i=0; i<6; i++) {
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
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		scene->draw_content(true);
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	Camera::Active = cached_camera;
}

void PointLight::set_cast_shadow(bool cast) {
	if (!cast) {
		cast_shadow = false;
		return;
	}
	if (shadow_map_initialized) {
		cast_shadow = true;
		return;
	}

	// cast == true; shadow map needs initialization
	cast_shadow = true;
	shadow_map_initialized = true;

	shadow_map_dim = 256;

	shadow_map_cam = new Camera(effective_radius*2, effective_radius*2, false, false);
	shadow_map_cam->lock();
	shadow_map_cam->cutoffNear = 0.5f;
	shadow_map_cam->fov = radians(90.0f);
	shadow_map_cam->aspect_ratio = 1.0f;

	//------- buffer generations -------

	// shadow map framebuffers
	glGenFramebuffers(6, shadow_map_fbos);

	// shadow map depth (non-readable)
	glGenRenderbuffers(1, &shadow_map_depth_rbo);
	glBindRenderbuffer(GL_RENDERBUFFER, shadow_map_depth_rbo);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, shadow_map_dim, shadow_map_dim);

	// generate cube map and bind each face to an fbo (to be rendered to later)
	glGenTextures(1, &shadow_map_tex); 
	glBindTexture(GL_TEXTURE_CUBE_MAP, shadow_map_tex);

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	for (int i=0; i<6; i++) {
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_R16F, 
				shadow_map_dim, shadow_map_dim, 0, GL_RED, GL_FLOAT, NULL);
		// set a face of cube map to an fbo
		glBindFramebuffer(GL_FRAMEBUFFER, shadow_map_fbos[i]);
		glFramebufferTexture2D(
				GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X+i, shadow_map_tex, 0);
		// set depth
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, shadow_map_depth_rbo);

		if (glCheckFramebufferStatus(GL_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE)
			ERR("point light shadow map framebuffer not correctly initialized");
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// shadow mask (output)
	int w = Program::Instance->drawable_width;
	int h = Program::Instance->drawable_height;

	glBindTexture(GL_TEXTURE_2D, shadow_mask_tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, w, h, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);
	new NamedTex(name + " shadow mask", shadow_mask_tex);

}

//-------- utilities --------

void Light::set_directional_shadowpass_params_for_mesh(Mesh* mesh, int shader_index) {
	mat4 o2w = mesh->object_to_world();
	mesh->shaders[shader_index].set_mat4("OBJECT_TO_WORLD", o2w);
	mesh->shaders[shader_index].set_mat4("OBJECT_TO_CLIP", Camera::Active->world_to_clip() * o2w);
	mesh->shaders[shader_index].set_mat3("OBJECT_TO_WORLD_ROT", mesh->object_to_world_rotation());

	Scene* scene = mesh->get_scene();
	int num_shadow_casters = 0;
	for (int i=0; i<scene->d_lights.size(); i++) {
		DirectionalLight* L = scene->d_lights[i];
		if (!L->get_cast_shadow()) continue;

		std::string prefix = "DirectionalLights[" + std::to_string(num_shadow_casters) + "].";
		mat4 OBJECT_TO_LIGHT_CLIP = L->world_to_light_clip() * o2w;
		mesh->shaders[shader_index].set_tex2D(prefix+"ShadowMap", i, L->get_shadow_map());
		mesh->shaders[shader_index].set_mat4(prefix+"OBJECT_TO_CLIP", OBJECT_TO_LIGHT_CLIP);
		mesh->shaders[shader_index].set_vec3(prefix+"Direction", L->get_direction());
		num_shadow_casters++;
	}
	mesh->shaders[shader_index].set_int("NumDirectionalLights", num_shadow_casters);
}

void Light::set_point_shadowpass_params_for_mesh(Mesh* mesh, int shader_index) {
	mat4 o2w = mesh->object_to_world();
	mesh->shaders[shader_index].set_mat4("OBJECT_TO_WORLD", o2w);
	mesh->shaders[shader_index].set_mat4("OBJECT_TO_CLIP", Camera::Active->world_to_clip() * o2w);
	mesh->shaders[shader_index].set_mat3("OBJECT_TO_WORLD_ROT", mesh->object_to_world_rotation());

	Scene* scene = mesh->get_scene();
	int num_shadow_casters = 0;
	for (int i=0; i<scene->p_lights.size(); i++) {
		PointLight* L = scene->p_lights[i];
		if (!L->get_cast_shadow()) continue;

		std::string prefix = "PointLights[" + std::to_string(i) + "].";
		mesh->shaders[shader_index].set_texCube(prefix+"ShadowMap", i, L->get_shadow_map());
		mesh->shaders[shader_index].set_vec3(prefix+"Position", L->world_position());
		num_shadow_casters++;
	}
	mesh->shaders[shader_index].set_int("NumPointLights", num_shadow_casters);
}

void PointLight::set_location_to_all_shaders(Scene* scene, int shader_index) {
	std::vector<Mesh*> meshes = scene->get_meshes();
	for (int i=0; i<meshes.size(); i++) {
  	glUseProgram(meshes[i]->shaders[shader_index].id);
		meshes[i]->shaders[shader_index].set_vec3("FIXED_POINT", world_position());
	}
	glUseProgram(0);
}
