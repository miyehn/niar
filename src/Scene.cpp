#include "Scene.hpp"
#include "Program.hpp"
#include "Globals.hpp"
#include "Mesh.hpp"
#include "Light.hpp"

CVar<int>* ShowDebugTex = new CVar<int>("ShowDebugTex", 0);
CVar<int>* DebugTex = new CVar<int>("DebugTex", 5);
CVar<float>* DebugTexMin = new CVar<float>("DebugTexMin", 0.0f);
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
  // blending: "blend the computed fragment color values with the values in the color buffers."
  blend = false; // NOTE: see no reason why this should be enabled for 3D scenes 
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

	glDrawBuffers(NUM_GBUFFERS, color_attachments_gbuffers);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE)
		ERR("G buffer framebuffer not correctly initialized");
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// Light-space position framebuffer
	
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
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	composition = new Blit("../shaders/deferred_composition.frag");

}

void Scene::update(float elapsed) {
}

void Scene::draw_content(bool shadow_pass) {
#if 1
	for (int i=0; i<children.size(); i++) {
		if (!shadow_pass) {
			children[i]->draw();
			continue;
		}
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
		children[i]->draw();
	}
#else
	Drawable::draw();
#endif
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
  // blending
  if (blend) glEnable(GL_BLEND);
  else glDisable(GL_BLEND);
  // fill mode
  glPolygonMode(GL_FRONT_AND_BACK, fill_mode);

	//---------------- actual drawing ----------------
	
	glViewport(0, 0, w, h);

	shader_set = ShaderSet->get();
	if (shader_set != 0) {
		draw_content();
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
	
	glViewport(0, 0, w, h);

	uint shadow_casting_lights_counter = 0;
	glBindFramebuffer(GL_FRAMEBUFFER, fbo_position_lights);
  glClearColor(1, 1, 1, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	// TODO: make lights manage separate fbos instead
	for (int i=0; i<lights.size(); i++) {
		if (shadow_casting_lights_counter > MAX_SHADOWCASTING_LIGHTS) ERR("Too many shadow casters");
		if (lights[i]->cast_shadow) {
			glFramebufferTexture2D(
					GL_FRAMEBUFFER, 
					color_attachments_position_lights[shadow_casting_lights_counter], 
					GL_TEXTURE_2D, 
					lights[i]->get_shadow_mask(), 0);

			shadow_casting_lights_counter++;
		}
	}
	shader_set = 3;
	draw_content();

	//-------- composition: copy to screen
	
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	
	composition->begin_pass();
	pass_lights_to_composition_shader();
	for (int i=0; i<NUM_GBUFFERS; i++) {
		std::string name = "GBUF" + std::to_string(i);
		composition->shader.set_tex2D(name, i, tex_gbuffers[i]);
	}
	composition->end_pass();

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

void Scene::pass_lights_to_composition_shader() {

	int dir_index = 0;
	int point_index = 0;
	int shadow_casting_light_index = 0;
	for (int i=0; i<lights.size(); i++) {
		
		if (DirectionalLight* L = dynamic_cast<DirectionalLight*>(lights[i])) {
			std::string prefix = "DirectionalLights[" + std::to_string(dir_index) + "].";
			composition->shader.set_vec3(prefix+"direction", L->get_direction());
			composition->shader.set_vec3(prefix+"color", L->get_emission());
			composition->shader.set_bool(prefix+"castShadow", L->cast_shadow);
			composition->shader.set_tex2D(prefix+"shadowMask", 
					NUM_GBUFFERS + shadow_casting_light_index, L->get_shadow_mask());
			dir_index++;
			if (L->cast_shadow) shadow_casting_light_index++;

		} else if (PointLight* L = dynamic_cast<PointLight*>(lights[i])) {
			std::string prefix = "PointLights[" + std::to_string(point_index) + "].";
			composition->shader.set_vec3(prefix+"position", L->world_position());
			composition->shader.set_vec3(prefix+"color", L->get_emission());
			point_index++;
			if (L->cast_shadow) shadow_casting_light_index++;

		}
	}
	composition->shader.set_int("NumDirectionalLights", dir_index);
	composition->shader.set_int("NumPointLights", point_index);
}

