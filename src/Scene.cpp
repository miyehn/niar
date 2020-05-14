#include "Scene.hpp"
#include "Program.hpp"
#include "Globals.hpp"

Scene::Scene(std::string _name) : Drawable(nullptr, _name) {

	int w, h;
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
	
	if (!Cfg.UseDeferred) return;
	
	glGenFramebuffers(1, &fbo_gbuffers);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo_gbuffers);

	// G buffer color attachments
	uint color_attachments[NUM_GBUFFERS];
	glGenTextures(NUM_GBUFFERS, tex_gbuffers);
	for (int i=0; i<NUM_GBUFFERS; i++) {
		glBindTexture(GL_TEXTURE_2D, tex_gbuffers[i]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		// filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		// attach to fbo
		color_attachments[i] = GL_COLOR_ATTACHMENT0+i;
		glFramebufferTexture2D(GL_FRAMEBUFFER, color_attachments[i], GL_TEXTURE_2D, tex_gbuffers[i], 0);
	}

	// depth renderbuffer
	glGenRenderbuffers(1, &buf_depth);
	glBindRenderbuffer(GL_RENDERBUFFER, buf_depth);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, w, h);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, buf_depth);

	glDrawBuffers(NUM_GBUFFERS, color_attachments);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE)
		ERR("framebuffer not correctly initialized");

	composition = new Blit("../shaders/deferred_composition.frag");

}

CVar<int>* GBuf = new CVar<int>("gbuf", 0);

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

	//-------- actual drawing --------
	
	if (!Cfg.UseDeferred) {
		Drawable::draw();
		return;
	}
	
	glBindFramebuffer(GL_FRAMEBUFFER, fbo_gbuffers);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// bind output textures
	for (int i=0; i<NUM_GBUFFERS; i++) {
		glActiveTexture(GL_TEXTURE0+i);
		glBindTexture(GL_TEXTURE_2D, tex_gbuffers[i]);
	}
	
	// draw scene
  Drawable::draw();

	// copy to screen
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	
	composition->begin_pass();
	{
		//composition->shader.set_tex2D(0, tex_gbuffers[GBuf->get()]);
		for (int i=0; i<NUM_GBUFFERS; i++) {
			std::string name = "GBUF" + std::to_string(i);
			composition->shader.set_tex2D(name, i, tex_gbuffers[i]);
		}
	}
	composition->end_pass();

}
