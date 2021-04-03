#include "Scene.hpp"
#include "Program.hpp"
#include "Input.hpp"
#include "Mesh.hpp"
#include "Light.hpp"
#include "Materials.hpp"
#include "Camera.hpp"

#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

Scene::Scene(std::string _name) : Drawable(nullptr, _name) {

	name = "[unnamed scene]";
}

void Scene::initialize_graphics() {

	SDL_GL_GetDrawableSize(Program::Instance->window, &w, &h);
	
	//-------- configurations --------
	// depth test
	use_depth_test = true;
	// culling
	cull_face = false;
	cull_mode = GL_BACK;
	// fill / wireframe (/ point)
	fill_mode = GL_FILL; // GL_FILL | GL_LINE | GL_POINT

	// https://github.com/nothings/stb/issues/335
	glPixelStorei(GL_UNPACK_ALIGNMENT, 2);
	
	//-------- allocate bufers --------
	
	// G buffers
	
	glGenFramebuffers(1, &fbo_gbuffers);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo_gbuffers);

	// G buffer color attachments
	glGenTextures(NUM_GBUFFERS, tex_gbuffers);
	for (int i=0; i<NUM_GBUFFERS; i++) {
		glBindTexture(GL_TEXTURE_2D, tex_gbuffers[i]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
		// filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		// attach to fbo
		color_attachments_gbuffers[i] = GL_COLOR_ATTACHMENT0+i;
		glFramebufferTexture2D(GL_FRAMEBUFFER, color_attachments_gbuffers[i], GL_TEXTURE_2D, tex_gbuffers[i], 0);
		// add to debug textures
		new NamedTex("GBUF"+std::to_string(i), tex_gbuffers[i]);
	}
	glDrawBuffers(NUM_GBUFFERS, color_attachments_gbuffers);

	// depth texture
	glGenTextures(1, &tex_depth);
	glBindTexture(GL_TEXTURE_2D, tex_depth);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, w, h, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, tex_depth, 0);
	new NamedTex("Depth", tex_depth);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER)!= GL_FRAMEBUFFER_COMPLETE)
		ERR("G buffer framebuffer not correctly initialized");
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// Light-space position framebuffer
	// TODO: make these per-light properties
	{
		glGenFramebuffers(1, &fbo_position_lights);
		glBindFramebuffer(GL_FRAMEBUFFER, fbo_position_lights);
		// color attachments (NOTE: they're not bound to any textures for now)
		for (int i=0; i<MAX_SHADOWCASTING_LIGHTS; i++) {
			color_attachments_position_lights[i] = GL_COLOR_ATTACHMENT0 + i;
		}
		glDrawBuffers(MAX_SHADOWCASTING_LIGHTS, color_attachments_position_lights);
		// depth renderbuffer
		glGenRenderbuffers(1, &depthbuf_position_lights);
		glBindRenderbuffer(GL_RENDERBUFFER, depthbuf_position_lights);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, w, h);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthbuf_position_lights);
		// finish
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE)
			ERR("Light-space position framebuffer not correctly initialized");
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	//---- setup scene color texture (to be ready for post processing)
	{
		glGenFramebuffers(1, &fbo_scene_color);
		glBindFramebuffer(GL_FRAMEBUFFER, fbo_scene_color);

		glGenTextures(1, &tex_scene_color);
		glBindTexture(GL_TEXTURE_2D, tex_scene_color);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		// single color attachment, no depth
		color_attachments_scene_color = GL_COLOR_ATTACHMENT0;
		glFramebufferTexture2D(GL_FRAMEBUFFER, color_attachments_scene_color, GL_TEXTURE_2D, tex_scene_color, 0);
		glDrawBuffers(1, &color_attachments_scene_color);
		// check error
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			ERR("scene color framebuffer not correctly initialized");
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	{ // and an alternative, in case of ping pong (currently for exposure adjustment & extract bright regions)
		glGenFramebuffers(1, &fbo_scene_color_alt);
		glBindFramebuffer(GL_FRAMEBUFFER, fbo_scene_color_alt);

		glGenTextures(2, tex_scene_colors_alt);
		for (int i=0; i<2; i++) {
			glBindTexture(GL_TEXTURE_2D, tex_scene_colors_alt[i]);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, w, h, 0, GL_RGB, GL_FLOAT, nullptr);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			// 2 color attachments, no depth
			color_attachments_scene_color_alt[i] = GL_COLOR_ATTACHMENT0 + i;
			glFramebufferTexture2D(GL_FRAMEBUFFER, color_attachments_scene_color_alt[i], GL_TEXTURE_2D, tex_scene_colors_alt[i], 0);
		}
		new NamedTex("Bright regions", tex_scene_colors_alt[1]);
		glDrawBuffers(2, color_attachments_scene_color_alt);
		// check error
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			ERR("alt scene color framebuffer not correctly initialized");
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	{ // gaussian pingpong
		glGenFramebuffers(2, fbo_gaussian_pingpong);
		glGenTextures(2, tex_gaussian_pingpong);
		for (int i=0; i<2; i++) {
			glBindFramebuffer(GL_FRAMEBUFFER, fbo_gaussian_pingpong[i]);
			glBindTexture(GL_TEXTURE_2D, tex_gaussian_pingpong[i]);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, w, h, 0, GL_RGB, GL_FLOAT, nullptr);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex_gaussian_pingpong[i], 0);
			if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
				ERR("gaussian pingpong[%d] framebuffer not correctly initialized", i);
			new NamedTex("Gaussian", tex_gaussian_pingpong[i]);
		}
	}
	GL_ERRORS();
}

// OMG MIND BLOWN: https://stackoverflow.com/questions/677620/do-i-need-to-explicitly-call-the-base-virtual-destructor
Scene::~Scene() {
}

void Scene::load(std::string source, bool preserve_existing_objects) {
	if (!preserve_existing_objects) {
		d_lights.clear();
		p_lights.clear();
		for (int i=0; i<children.size(); i++) delete children[i];
		children.clear();
	}

	LOG("-------- loading scene --------");
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(source,
			// aiProcess_GenSmoothNormals
			aiProcess_CalcTangentSpace
			| aiProcess_Triangulate
			| aiProcess_FlipUVs
			// | aiProcess_JoinIdenticalVertices
			// | aiProcess_SortByPType
			);
	if (!scene) {
		ERR("%s", importer.GetErrorString());
	}
	LOG(" - %d meshes", scene->mNumMeshes);
	LOG(" - %d lights", scene->mNumLights);
	//LOG(" - %d cameras", scene->mNumCameras);

	// meshes
	for (int i=0; i<scene->mNumMeshes; i++) {
		aiMesh* mesh = scene->mMeshes[i];
		if (mesh) {
			Mesh* m = new Mesh(mesh);
			m->initialize();
			add_child(m);
		}
	}
	generate_aabb();

	// lights
	for (int i=0; i<scene->mNumLights; i++) {
		aiLight* light = scene->mLights[i];
		if (light->mType == aiLightSource_DIRECTIONAL) {
			DirectionalLight* d_light = new DirectionalLight(light, scene->mRootNode);
			d_lights.push_back(d_light);
			d_light->set_cast_shadow(true);
			add_child(d_light);
			LOG("loaded directional light '%s', color: %s", d_light->name.c_str(), s3(d_light->get_emission()).c_str());
			
		} else if (light->mType == aiLightSource_POINT) {
			PointLight* p_light = new PointLight(light, scene->mRootNode);
			p_lights.push_back(p_light);
			p_light->set_cast_shadow(true);
			add_child(p_light);
			LOG("loaded point light '%s', color: %s", p_light->name.c_str(), s3(p_light->get_emission()).c_str());

		} else {
			WARN("unrecognized light type, skipping..");
		}
	}

	LOG("-------------------------------");
}

// TODO: make this support parenting hierarchy
std::vector<Mesh*> Scene::get_meshes() {
	std::vector<Mesh*> meshes;
	for (int i=0; i<children.size(); i++) {
		if (Mesh* mesh = dynamic_cast<Mesh*>(children[i])) meshes.push_back(mesh);
	}
	return meshes;
}

void Scene::generate_aabb() {
	std::vector<Mesh*> meshes = get_meshes();
	aabb = AABB();
	for (int i=0; i<meshes.size(); i++) {
		aabb.merge(meshes[i]->aabb);
	}
}

// TODO: make this support parenting hierarchy
void Scene::draw_content(bool shadow_pass) {
	for (int i=0; i<children.size(); i++) {
#if 0 // front face culling
		if (!shadow_pass) {
			children[i]->draw();
			continue;
		}
		//---- shadow pass ----
		// for each child of type Mesh and is closed mesh
		if (Mesh* mesh = dynamic_cast<Mesh*>(children[i])) {
			if (!mesh->is_thin_mesh) {
				glEnable(GL_CULL_FACE);
				glCullFace(GL_FRONT);
				mesh->draw();
				glCullFace(cull_mode);
				if (!cull_face) glDisable(GL_CULL_FACE);
				continue;
			}
		}
		// non-mesh or not-closed mesh
		children[i]->draw();
#else
		children[i]->draw();
#endif
	}
}

void Scene::draw() {

	//-------- setup config --------
	// depth test
	if (use_depth_test) glEnable(GL_DEPTH_TEST);
	else glDisable(GL_DEPTH_TEST);
	// culling
	if (cull_face) {
		glEnable(GL_CULL_FACE);
		glCullFace(cull_mode);
	} else {
		glDisable(GL_CULL_FACE);
	}
	// fill mode
	glPolygonMode(GL_FRONT_AND_BACK, fill_mode);

	//---------------- actual drawing ----------------
	
	glViewport(0, 0, w, h);

	material_set = Cfg.MaterialSet->get();
	if (material_set == 0) {
		draw_content();
		// if it's not drawing deferred base pass, skip the rest of deferred pipeline
		return;
	}

	//-------- draw geometry information, independent of lighting

	glBindFramebuffer(GL_FRAMEBUFFER, fbo_gbuffers);
	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	// bind output textures
	for (int i=0; i<NUM_GBUFFERS; i++) {
		glActiveTexture(GL_TEXTURE0+i);
		glBindTexture(GL_TEXTURE_2D, tex_gbuffers[i]);
	}
	// draw scene to G buffer
	draw_content();
	
	//-------- make shadow maps
	
	for (int i=0; i<d_lights.size(); i++) {
		d_lights[i]->render_shadow_map();
	}
	for (int i=0; i<p_lights.size(); i++) {
		p_lights[i]->render_shadow_map();
	}

	//-------- draw shadow masks (in a one-pass MRT) by sampling shadow maps
	// if a light doesn't cast shadow, will NOT draw to its shadow mask (it will contain garbage data)
	
	glViewport(0, 0, w, h);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo_position_lights);

	//---- directional lights
	// configure output buffers
	int shadow_caster_counter = 0;
	for (int i=0; i<d_lights.size(); i++) {
		DirectionalLight* L = d_lights[i];
		if (!L->get_cast_shadow()) continue;

		glFramebufferTexture2D(
				GL_FRAMEBUFFER, 
				color_attachments_position_lights[shadow_caster_counter], 
				GL_TEXTURE_2D, 
				L->get_shadow_mask(), 0);

		shadow_caster_counter++;
	}
	glClearColor(1, 1, 1, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// then blit
	Blit* blit = Blit::shadow_mask_directional();
	blit->begin_pass();
	{
		int num_shadow_casters = 0;
		for (int i=0; i<d_lights.size(); i++) {
			DirectionalLight* L = d_lights[i];
			if (!L->get_cast_shadow()) continue;
			std::string prefix = "DirectionalLights[" + std::to_string(num_shadow_casters) + "].";
			blit->set_mat4(prefix + "WORLD_TO_LIGHT_CLIP", L->world_to_light_clip());
			blit->set_tex2D(prefix + "ShadowMap", num_shadow_casters, L->get_shadow_map());
			blit->set_vec3(prefix + "Direction", L->get_direction());
			num_shadow_casters++;
		}
		blit->set_tex2D("Position", num_shadow_casters, tex_gbuffers[0]);
		blit->set_tex2D("Normal", num_shadow_casters+1, tex_gbuffers[1]);
		blit->set_int("NumDirectionalLights", num_shadow_casters);
	}
	blit->end_pass();

	//---- point lights
	// configure output buffers
	shadow_caster_counter = 0;
	for (int i=0; i<p_lights.size(); i++) {
		PointLight* L = p_lights[i];
		if (!L->get_cast_shadow()) continue;

		glFramebufferTexture2D(
				GL_FRAMEBUFFER, 
				color_attachments_position_lights[shadow_caster_counter], 
				GL_TEXTURE_2D, 
				L->get_shadow_mask(), 0);

		shadow_caster_counter++;
	}
	glClearColor(1, 1, 1, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	blit = Blit::shadow_mask_point();
	blit->begin_pass();
	{
		int num_shadow_casters = 0;
		for (int i=0; i<p_lights.size(); i++) {
			PointLight* L = p_lights[i];
			if (!L->get_cast_shadow()) continue;
			std::string prefix = "PointLights[" + std::to_string(num_shadow_casters) + "].";
			blit->set_vec3(prefix + "Position", L->world_position());
			blit->set_texCube(prefix + "ShadowMap", num_shadow_casters, L->get_shadow_map());
			num_shadow_casters++;
		}
		blit->set_tex2D("Position", num_shadow_casters, tex_gbuffers[0]);
		blit->set_tex2D("Normal", num_shadow_casters+1, tex_gbuffers[1]);
		blit->set_int("NumPointLights", num_shadow_casters);
	}
	blit->end_pass();

	//-------- lighting passes --> scene color texture
	
	glBindFramebuffer(GL_FRAMEBUFFER, fbo_scene_color);
	
	// directional lights
	blit = Blit::lighting_directional();
	blit->begin_pass();
	{
		// g buffers
		for (int i=0; i<NUM_GBUFFERS; i++) {
			std::string name = "GBUF" + std::to_string(i);
			blit->set_tex2D(name, i, tex_gbuffers[i]);
		}
		// light-related
		for (int i=0; i<d_lights.size(); i++) {
			DirectionalLight* L = d_lights[i];
			std::string prefix = "DirectionalLights[" + std::to_string(i) + "].";
			blit->set_vec3(prefix+"direction", L->get_direction());
			blit->set_vec3(prefix+"color", L->get_emission());
			blit->set_bool(prefix+"castShadow", L->get_cast_shadow());
			blit->set_tex2D(prefix+"shadowMask", NUM_GBUFFERS + i, L->get_shadow_mask());
		}
		blit->set_int("NumDirectionalLights", d_lights.size());

		// camera
		blit->set_vec3("CameraPosition", Camera::Active->position);
	}
	blit->end_pass();

	// point lights
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);

	blit = Blit::lighting_point();
	blit->begin_pass();
	{
		// g buffers
		for (int i=0; i<NUM_GBUFFERS; i++) {
			std::string name = "GBUF" + std::to_string(i);
			blit->set_tex2D(name, i, tex_gbuffers[i]);
		}
		// light-related
		for (int i=0; i<p_lights.size(); i++) {
			PointLight* L = p_lights[i];
			std::string prefix = "PointLights[" + std::to_string(i) + "].";
			blit->set_vec3(prefix+"position", L->world_position());
			blit->set_vec3(prefix+"color", L->get_emission());
			blit->set_bool(prefix+"castShadow", L->get_cast_shadow());
			blit->set_tex2D(prefix+"shadowMask", NUM_GBUFFERS + i, L->get_shadow_mask());
		}
		blit->set_int("NumPointLights", p_lights.size());

		// camera
		blit->set_vec3("CameraPosition", Camera::Active->position);
	}
	blit->end_pass();

	//-------- post processing: exposure adjustment & extracting bright region --> scene_color_alt
	glDisable(GL_BLEND);

	glBindFramebuffer(GL_FRAMEBUFFER, fbo_scene_color_alt);
	blit = Blit::exposure_extract_bright();
	blit->begin_pass();
	{
		blit->set_tex2D("TEX", 0, tex_scene_color);
		blit->set_float("Exposure", Cfg.Exposure->get());
		blit->set_float("BloomThreshold", Cfg.BloomThreshold->get());
	}
	blit->end_pass();

	//-------- post processing: gaussian pingpong --> gaussian_pingpong[2]

	bool bloom = Cfg.Bloom->get();
	if (bloom) {
		// horizontal
		glBindFramebuffer(GL_FRAMEBUFFER, fbo_gaussian_pingpong[0]);
		blit = Blit::gaussian_horizontal();
		blit->begin_pass();
		{
			blit->set_tex2D("TEX", 0, tex_scene_colors_alt[1]);
			blit->set_float("InvWidth", 1.0f / w);
		}
		blit->end_pass();

		// vertical
		glBindFramebuffer(GL_FRAMEBUFFER, fbo_gaussian_pingpong[1]);
		blit = Blit::gaussian_vertical();
		blit->begin_pass();
		{
			blit->set_tex2D("TEX", 0, tex_gaussian_pingpong[0]);
			blit->set_float("InvHeight", 1.0f / h);
		}
		blit->end_pass();
	}

	//-------- final post processing (combine bloom & tone mapping & gamma correction) --> screen

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	blit = Blit::tone_map_gamma_correct();
	blit->begin_pass();
	{
		blit->set_tex2D("TEX", 0, tex_scene_colors_alt[0]);
		blit->set_tex2D("TEX", 1, bloom ? tex_gaussian_pingpong[1] : Texture::black()->id());
		blit->set_int("ToneMapping", Cfg.ToneMapping->get());
		blit->set_int("GammaCorrect", Cfg.GammaCorrect->get());
	}
	blit->end_pass();

	// draw debug texture on top
	if (Cfg.ShowDebugTex->get()) {
		int debugtex = find_named_tex(Cfg.DebugTex->get());
		if (debugtex >= 0) {
			Blit::copy_debug()->begin_pass();
			Blit::copy_debug()->set_tex2D("TEX", 0, debugtex);
			Blit::copy_debug()->set_vec2("MinMax", vec2(Cfg.DebugTexMin->get(), Cfg.DebugTexMax->get()));
			Blit::copy_debug()->end_pass();
		}
	}

	GL_ERRORS();
}

