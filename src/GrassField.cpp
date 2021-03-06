#include "GrassField.hpp"
#include "Camera.hpp"
#include "Shader.hpp"
#include "Program.hpp"
#include "Materials.hpp"

GrassField::GrassField(uint num_blades, Drawable* _parent, std::string _name): Drawable(_parent, _name) {

	// create blades
	blades = std::vector<Blade>();
	for (uint i=0; i<num_blades; i++) {
		Blade b = Blade(vec3(-10.0f + i*3.0f, 0.0f, 0.0f));
		blades.push_back(b);
	}

	{ // create vao and shaders
		// generate 1 vbo
		glGenBuffers(1, &vbo);
		// generate 1 vao, bind it (start refering to it)
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);
		// bind vbo to current vao (start refering to it)
		glBindBuffer(GL_ARRAY_BUFFER, vbo);

		// vertex attrib pointers:
		glVertexAttribPointer(
				0, // index of ptr (attribute)
				4, // size
				GL_FLOAT, // type
				GL_FALSE, // normalized
				4*sizeof(float), // stride size
				(void*)0 // offset
		);
		glEnableVertexAttribArray(0); // enable vertex attrib ptr at index 0

		// specify patch size for tesselation
		glPatchParameteri(GL_PATCH_VERTICES, 4);

		// done refering to vbo and vao, unbind them
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);

		// shaders
		material = new MatGrass();
	}
}

GrassField::~GrassField() {
	if (material) delete material;
	glDeleteBuffers(1, &vbo);
	vbo = 0;
	glDeleteVertexArrays(1, &vao);
	vao = 0;
}

void GrassField::update(float elapsed) {
	Drawable::update(elapsed);
}

bool GrassField::handle_event(SDL_Event event) {
	return Drawable::handle_event(event);
}

void GrassField::draw() {

	// upload render_buffer
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, blades.size() * sizeof(Blade), blades.data(), GL_STREAM_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// set material
	material->use(this);

	// use vao
	glBindVertexArray(vao);

	// draw.
	glDrawArrays(GL_PATCHES, 0, 4 * blades.size());

	// unbind vao
	glBindVertexArray(0);

	// reset shader
	glUseProgram(0);

	Drawable::draw();
	
}

void GrassField::set_local_position(vec3 _local_position) {
	local_position_value = _local_position;
	/*
	generate_aabb();
	get_scene()->generate_aabb();
	*/
}

void GrassField::set_rotation(quat _rotation) {
	rotation_value = _rotation;
	/*
	generate_aabb();
	get_scene()->generate_aabb();
	*/
}

void GrassField::set_scale(vec3 _scale) {
	scale_value = _scale;
	/*
	generate_aabb();
	get_scene()->generate_aabb();
	*/
}


Blade::Blade(vec3 root) {
	this->up_o = vec4(0.0f, 0.0f, 1.0f, radians(65.0f));

	this->root_w = vec4(root.x, root.y, root.z, 0.65f);

	this->above_h = this->up_o * 4.0f;
	this->above_h.w = 1.0f;

	this->ctrl_s = above_h + vec4(-0.8f, 0.0f, 0.0f, 0.0f);
	this->ctrl_s.w = 1.0f;
}
