#include "Scene.hpp"
#include "Program.hpp"
#include "Globals.hpp"
#include "Mesh.hpp"
#include "Light.hpp"

CVar<int>* ShowDebugTex = new CVar<int>("ShowDebugTex", 0);
CVar<int>* DebugTex = new CVar<int>("DebugTex", 8);
CVar<float>* DebugTexMin = new CVar<float>("DebugTexMin", 0.6f);
CVar<float>* DebugTexMax = new CVar<float>("DebugTexMax", 1.0f);

CVar<int>* ShaderSet = new CVar<int>("ShaderSet", 0);

Scene::Scene(std::string _name) : Drawable(nullptr, _name) {

	SDL_GL_GetDrawableSize(Program::Instance->window, &w, &h);
	
	//-------- configurations --------
  // depth test
  use_depth_test = true;
  // culling
  cull_face = false;
  cull_mode = GL_BACK;
  // fill / wireframe (/ point)
  fill_mode = GL_FILL; // GL_FILL | GL_LINE | GL_POINT
  
	//-------- allocate bufers --------
	
	// G buffer
	
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

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE)
		ERR("G buffer framebuffer not correctly initialized");
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// Light-space position framebuffer
	// TODO: make these per-light properties
	
	glGenFramebuffers(1, &fbo_position_lights);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo_position_lights);
	{
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

	lighting_directional = new Blit("../shaders/deferred_lighting_directional.frag");
	lighting_point = new Blit("../shaders/deferred_lighting_point.frag");

}

void Scene::update(float elapsed) {

	d_lights.clear();
	ds_lights.clear();
	p_lights.clear();
	ps_lights.clear();

	for (int i=0; i<lights.size(); i++) {
		if (DirectionalLight* DL = dynamic_cast<DirectionalLight*>(lights[i])) {
			d_lights.push_back(DL);
			if (DL->get_cast_shadow()) {
				ds_lights.push_back(DL);
			}
		}
		else if (PointLight* PL = dynamic_cast<PointLight*>(lights[i])) {
			p_lights.push_back(PL);
			if (PL->get_cast_shadow()) {
				ps_lights.push_back(PL);
			}
		}
	}

}

void Scene::draw_content(bool shadow_pass) {
	for (int i=0; i<children.size(); i++) {
		if (!shadow_pass) {
			children[i]->draw();
			continue;
		}
		//---- shadow pass ----
		// for each child of type Mesh and is closed mesh
		if (Mesh* mesh = dynamic_cast<Mesh*>(children[i])) {
			if (mesh->is_closed_mesh) {
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

	shader_set = ShaderSet->get();
	if (shader_set != 0) {
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
	shader_set = 0;
	draw_content();
	
	//-------- make shadow maps
	
	for (int i=0; i<lights.size(); i++) {
		lights[i]->render_shadow_map();
	}

	//-------- draw shadow masks (in a one-pass MRT) by sampling shadow maps
	// if a light doesn't cast shadow, will NOT draw to its shadow mask (it will contain garbage data)
	
	glViewport(0, 0, w, h);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo_position_lights);

	//---- directional lights
	uint shadow_caster_counter = 0;
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
	// set the rest of output buffers to dummy (necessary?)
	for (int i=shadow_caster_counter; i<MAX_SHADOWCASTING_LIGHTS; i++) {
		glFramebufferTexture2D(
				GL_FRAMEBUFFER, 
				color_attachments_position_lights[i], 
				GL_TEXTURE_2D, 
				0, 0);
	}
  glClearColor(1, 1, 1, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	shader_set = 3;
	draw_content();

	//---- point lights
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
	for (int i=shadow_caster_counter; i<MAX_SHADOWCASTING_LIGHTS; i++) {
		glFramebufferTexture2D(
				GL_FRAMEBUFFER, 
				color_attachments_position_lights[i], 
				GL_TEXTURE_2D, 
				0, 0);
	}
  glClearColor(1, 1, 1, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	shader_set = 4;
	draw_content();

	//-------- lighting passes: copy to screen
	
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	
	// directional lights
	lighting_directional->begin_pass();
	for (int i=0; i<NUM_GBUFFERS; i++) {
		std::string name = "GBUF" + std::to_string(i);
		lighting_directional->shader.set_tex2D(name, i, tex_gbuffers[i]);
	}
	pass_directional_lights_to_lighting_shader();
	lighting_directional->end_pass();

	// point lights (TODO)
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);

	lighting_point->begin_pass();
	for (int i=0; i<NUM_GBUFFERS; i++) {
		std::string name = "GBUF" + std::to_string(i);
		lighting_point->shader.set_tex2D(name, i, tex_gbuffers[i]);
	}
	pass_point_lights_to_lighting_shader();
	lighting_point->end_pass();

	glDisable(GL_BLEND);

	// draw debug texture
	if (ShowDebugTex->get()) {
		int debugtex = find_named_tex(DebugTex->get());
		if (debugtex >= 0) {
			Blit::CopyDebug->begin_pass();
			Blit::CopyDebug->shader.set_tex2D("TEX", 0, debugtex);
			Blit::CopyDebug->shader.set_vec2("MinMax", vec2(DebugTexMin->get(), DebugTexMax->get()));
			Blit::CopyDebug->end_pass();
		}
	}

	GL_ERRORS();
}

void Scene::pass_directional_lights_to_lighting_shader() {
	for (int i=0; i<d_lights.size(); i++) {
		DirectionalLight* L = d_lights[i];
		std::string prefix = "DirectionalLights[" + std::to_string(i) + "].";
		lighting_directional->shader.set_vec3(prefix+"direction", L->get_direction());
		lighting_directional->shader.set_vec3(prefix+"color", L->get_emission());
		lighting_directional->shader.set_bool(prefix+"castShadow", L->get_cast_shadow());
		lighting_directional->shader.set_tex2D(prefix+"shadowMask", NUM_GBUFFERS + i, L->get_shadow_mask());
	}
	lighting_directional->shader.set_int("NumDirectionalLights", d_lights.size());
}

void Scene::pass_point_lights_to_lighting_shader() {
	for (int i=0; i<p_lights.size(); i++) {
		PointLight* L = p_lights[i];
		std::string prefix = "PointLights[" + std::to_string(i) + "].";
		lighting_point->shader.set_vec3(prefix+"position", L->world_position());
		lighting_point->shader.set_vec3(prefix+"color", L->get_emission());
		lighting_point->shader.set_bool(prefix+"castShadow", L->get_cast_shadow());
		lighting_point->shader.set_tex2D(prefix+"shadowMask", NUM_GBUFFERS + i, L->get_shadow_mask());
	}
	lighting_point->shader.set_int("NumPointLights", p_lights.size());
}
