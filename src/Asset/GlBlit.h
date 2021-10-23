#pragma once

#include "Shader.h"

class GlBlit : public Shader {
public:

	// debug
	CONST_PTR(GlBlit, copy_debug);
	CONST_PTR(GlBlit, blit);

	// post processing
	CONST_PTR(GlBlit, exposure_extract_bright);
	CONST_PTR(GlBlit, tone_map_gamma_correct);
	CONST_PTR(GlBlit, gaussian_horizontal);
	CONST_PTR(GlBlit, gaussian_vertical);

	// shadow mapping
	CONST_PTR(GlBlit, shadow_mask_directional);
	CONST_PTR(GlBlit, shadow_mask_point);

	// lighting
	CONST_PTR(GlBlit, lighting_directional);
	CONST_PTR(GlBlit, lighting_point);

	GlBlit(const std::string& shader_name);

	void begin_pass();
	void end_pass();

private:
	static uint vao;
	static uint vbo;
	GLboolean cached_depth_test;

};
