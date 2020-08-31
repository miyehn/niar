#include "Light.hpp"
#include "Camera.hpp"
#include "Input.hpp"
#include "Scene.hpp"
#include "Program.hpp"
#include "Mesh.hpp"
#include "Materials.hpp"

#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

// TODO
Light::~Light() {
	delete shadow_map_cam;
}

void DirectionalLight::init(vec3 _color, float _intensity, vec3 dir) {
	type = Directional;
	
	color = _color;
	intensity = _intensity;
	set_direction(dir);

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

	Scene* scene = get_scene();
	
	// set camera properties
	
	shadow_map_cam->rotation = rotation();
	shadow_map_cam->position = vec3(0); // temporary

	// x, y bounds: compute both frustum-based and scene-based, take the smaller
	Frustum fr = Camera::Active->frustum();
	float minx_fr = INF; float maxx_fr = -INF;
	float miny_fr = INF; float maxy_fr = -INF;
	mat4 w2c0 = shadow_map_cam->world_to_camera();
	for (int i=0; i<8; i++) {
		vec4 v = w2c0 * vec4(fr[i], 1);
		minx_fr = min(minx_fr, v.x);
		maxx_fr = max(maxx_fr, v.x);
		miny_fr = min(miny_fr, v.y);
		maxy_fr = max(maxy_fr, v.y);
	}
	// z bounds: currently based on scene aabb; could further optimize by clipping it with view frustum
	std::vector<vec3> sc = scene->aabb.corners();
	float minx_sc = INF; float maxx_sc = -INF;
	float miny_sc = INF; float maxy_sc = -INF;
	float minz = INF; float maxz = -INF;
	for (int i=0; i<8; i++) {
		vec4 v = w2c0 * vec4(sc[i], 1);
		minx_sc = min(minx_sc, v.x);
		maxx_sc = max(maxx_sc, v.x);
		miny_sc = min(miny_sc, v.y);
		maxy_sc = max(maxy_sc, v.y);
		minz = min(minz, v.z);
		maxz = max(maxz, v.z);
	}
	float minx = max(minx_fr, minx_sc);
	float maxx = min(maxx_fr, maxx_sc);
	float miny = max(miny_fr, miny_sc);
	float maxy = min(maxy_fr, maxy_sc);
	
	// now compute the actual location
	mat4 c2w0 = shadow_map_cam->camera_to_world();
	vec3 position0 = vec3( (minx + maxx)/2, (miny+maxy)/2, maxz + 1.0f );
	shadow_map_cam->position = vec3(c2w0 * vec4(position0, 1));

	shadow_map_cam->width = maxx - minx;
	shadow_map_cam->height = maxy - miny;

	shadow_map_cam->cutoffFar = maxz - minz + 1.0f;
	shadow_map_cam->cutoffNear = 0.5f;

	// prepare to draw the scene
	Camera* cached_camera = Camera::Active;
	Camera::Active = shadow_map_cam;

	glBindFramebuffer(GL_FRAMEBUFFER, shadow_map_fbo);
	glClear(GL_DEPTH_BUFFER_BIT);

	glViewport(0, 0, shadow_map_dim, shadow_map_dim);
	scene->replacement_material = Material::mat_depth();
	scene->draw_content(true);
	scene->replacement_material = nullptr;

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

	shadow_map_cam = new Camera(1, 1, true, false);
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
	new NamedTex(name + " shadow map", shadow_map_tex);
	new NamedTex(name + " shadow mask", shadow_mask_tex);
}

//-------- point light --------

void PointLight::init(vec3 _color, float _intensity, vec3 _local_pos) {

	type = Point;

	color = _color;
	intensity = _intensity;

	float factor = 1.0f;
	if (color.r > 1 || color.g > 1 || color.b > 1) {
		factor = max(color.r, max(color.g, color.b));
		color *= (1.0f / factor);
		intensity = factor;
	}

	set_local_position(_local_pos);

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

	float effective_radius = sqrt(255.0f * intensity); // a heuristic

	shadow_map_cam->position = world_position();
	shadow_map_cam->cutoffFar = effective_radius;

	Camera* cached_camera = Camera::Active;
	Camera::Active = shadow_map_cam;

	Scene* scene = get_scene();
	glViewport(0, 0, shadow_map_dim, shadow_map_dim);

#define POINT_LIGHT_OPTIMIZE 1

	// TODO: merge these 6 calls into one (using MRT)
#if POINT_LIGHT_OPTIMIZE
	// dynamic bounds based on scene aabb and light effective radius (can also add frustum)
	std::vector<vec3> sc = scene->aabb.corners();
#endif
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

#if POINT_LIGHT_OPTIMIZE
		// z far bound (is this effective at all??)
		float minz = INF;
		mat4 w2c0 = shadow_map_cam->world_to_camera();
		for (int i=0; i<8; i++) {
			vec3 v = w2c0 * vec4(sc[i], 1);
			minz = min(minz, v.z);
		}
		shadow_map_cam->cutoffFar = min(-minz, effective_radius);
#endif

		// draw
		glBindFramebuffer(GL_FRAMEBUFFER, shadow_map_fbos[i]);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		scene->replacement_material = distance_to_light_mat;
		scene->draw_content(true);
		scene->replacement_material = nullptr;
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

	shadow_map_cam = new Camera(0, 0, false, false);
	shadow_map_cam->lock();
	shadow_map_cam->cutoffNear = 0.1f;
	shadow_map_cam->fov = radians(90.0f);
	shadow_map_cam->aspect_ratio = 1.0f;

	distance_to_light_mat = new MatGeneric("distance");
	distance_to_light_mat->set_parameters = [this]() {
		distance_to_light_mat->shader->set_vec3("FIXED_POINT", world_position());
	};

	//------- buffer generation -------

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

