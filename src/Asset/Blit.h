#pragma once

#include "Shader.hpp"

class Blit : public Shader {
public:

	// debug
	CONST_PTR(Blit, copy_debug);
	CONST_PTR(Blit, blit);

	// post processing
	CONST_PTR(Blit, exposure_extract_bright);
	CONST_PTR(Blit, tone_map_gamma_correct);
	CONST_PTR(Blit, gaussian_horizontal);
	CONST_PTR(Blit, gaussian_vertical);

	// shadow mapping
	CONST_PTR(Blit, shadow_mask_directional);
	CONST_PTR(Blit, shadow_mask_point);

	// lighting
	CONST_PTR(Blit, lighting_directional);
	CONST_PTR(Blit, lighting_point);

	Blit(const std::string& shader_name);

	void begin_pass();
	void end_pass();

private:
	static uint vao;
	static uint vbo;
	GLboolean cached_depth_test;

};
