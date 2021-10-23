#include "GlBlit.h"

//-------------------- Blit -------------------------

float n = 1.0f;
std::vector<float> quad_vertices = {
	-n, n, 0, 1, // tl
	-n, -n, 0, 0, // bl
	n, -n, 1, 0, // br
	-n, n, 0, 1, // tl
	n, -n, 1, 0, // br
	n, n, 1, 1 // tr
};

uint GlBlit::vao = 0;
uint GlBlit::vbo = 0;

GlBlit::GlBlit(const std::string& frag_path) : Shader(frag_path) {

	if (!GlBlit::vao || !GlBlit::vbo) {
		glGenBuffers(1, &GlBlit::vbo);
		glGenVertexArrays(1, &GlBlit::vao);
		glBindVertexArray(GlBlit::vao);
		{
			glBindBuffer(GL_ARRAY_BUFFER, GlBlit::vbo);
			glBufferData(GL_ARRAY_BUFFER,
						 quad_vertices.size() * sizeof(float), quad_vertices.data(), GL_STATIC_DRAW);

			glVertexAttribPointer(
				0, // atrib index
				2, // num of data elems
				GL_FLOAT, // data type
				GL_FALSE, // normalized
				4 * sizeof(float), // stride size
				(void*)0); // offset in bytes since stride start
			glEnableVertexAttribArray(0);

			glVertexAttribPointer(
				1, // atrib index
				2, // num of data elems
				GL_FLOAT, // data type
				GL_FALSE, // normalized
				4 * sizeof(float), // stride size
				(void*)(2 * sizeof(float))); // offset in bytes since stride start
			glEnableVertexAttribArray(1);
		}
		glBindVertexArray(0);

	}
}

void GlBlit::begin_pass() {
	glGetBooleanv(GL_DEPTH_TEST, &cached_depth_test);
	glDisable(GL_DEPTH_TEST);
	glUseProgram(id);
	glBindVertexArray(GlBlit::vao);
	glBindBuffer(GL_ARRAY_BUFFER, GlBlit::vbo);
}

void GlBlit::end_pass() {
	glDrawArrays(GL_TRIANGLES, 0, 6);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	glUseProgram(0);
	if (cached_depth_test == GL_TRUE) glEnable(GL_DEPTH_TEST);
}

#define IMPLEMENT_BLIT(NAME, SHADER_NAME) \
	GlBlit* GlBlit::NAME##_value = nullptr; \
	GlBlit* GlBlit::NAME() { \
		if (NAME##_value) return NAME##_value; \
		NAME##_value = dynamic_cast<GlBlit*>(Shader::get(SHADER_NAME)); \
		if (!NAME##_value) ERR(SHADER_NAME" blit is not correctly loaded!"); \
		return NAME##_value; \
	}

IMPLEMENT_BLIT(copy_debug, "copyDebug");
IMPLEMENT_BLIT(blit, "blit");

IMPLEMENT_BLIT(shadow_mask_directional, "shadowMaskDirectional");
IMPLEMENT_BLIT(shadow_mask_point, "shadowMaskPoint");
IMPLEMENT_BLIT(lighting_directional, "lightingDirectional");
IMPLEMENT_BLIT(lighting_point, "lightingPoint");

IMPLEMENT_BLIT(exposure_extract_bright, "exposureExtractBright");
IMPLEMENT_BLIT(tone_map_gamma_correct, "toneMapGammaCorrect");
IMPLEMENT_BLIT(gaussian_horizontal, "gaussianHorizontal");
IMPLEMENT_BLIT(gaussian_vertical, "gaussianVertical");
