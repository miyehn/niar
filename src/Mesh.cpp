#include "Mesh.hpp"
#include "Shader.hpp"
#include "Camera.hpp"
#include "Pathtracer/BSDF.hpp"
#include "Input.hpp"
#include "Scene.hpp"
#include "Light.hpp"
#include "Materials.hpp"
#include "Texture.hpp"

#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

Mesh::Mesh() {}

std::unordered_map<std::string, std::string> Mesh::material_assignment;

void Mesh::set_material_name_for(const std::string& mesh_name, const std::string& mat_name) {
	material_assignment[mesh_name] = mat_name;
}

std::string Mesh::get_material_name_for(const std::string& mesh_name) {
	auto pair = material_assignment.find(mesh_name);
	if (pair == material_assignment.end()) {
		return "";
	}
	return pair->second;
}

Mesh::Mesh(aiMesh* mesh, Drawable* _parent, std::string _name) : Drawable(_parent, _name) {

	if (!mesh->HasPositions() || !mesh->HasFaces() || !mesh->HasNormals()) {
		ERR("creating a mesh that has some data missing. skipping..");
		return;
	}

	// name
	const char* inName = mesh->mName.C_Str();
	if (_name == "[unnamed mesh]") name = inName;

	// thin mesh (from name)
	if (std::string(inName).substr(0, 5) == "thin_") {
		is_thin_mesh = true;
	} else is_thin_mesh = false;

	// iterate through vertices
	for (int i=0; i<mesh->mNumVertices; i++) {
		// access p, n, t data
		aiVector3D position = mesh->mVertices[i];
		aiVector3D normal = mesh->mNormals[i];
		aiVector3D uv = mesh->mTextureCoords[0][i];
		aiVector3D tangent = mesh->mTangents[i];
		// create vertex from pnc
		Vertex v;
		v.position = vec3(position.x, position.y, position.z);
		v.normal = vec3(normal.x, normal.y, normal.z);
		v.tangent = vec3(tangent.x, tangent.y, tangent.z);
		v.tangent = normalize(v.tangent - dot(v.tangent, v.normal) * v.normal); // gram-schmidt
		v.uv = vec2(uv.x, uv.y);
		vertices.push_back(v);
	}

	// iterate through faces indices and store them
	for (int j=0; j<mesh->mNumFaces; j++) {
		aiFace face = mesh->mFaces[j];
		int i1 = face.mIndices[0];
		int i2 = face.mIndices[1];
		int i3 = face.mIndices[2];
		faces.push_back(i1);
		faces.push_back(i2);
		faces.push_back(i3);
	}

	// initialize();
}

void Mesh::initialize() {

	generate_aabb();

	//---- OpenGL setup ----

	// materials
	for (int i=0; i<NUM_MATERIAL_SETS; i++) materials[i] = nullptr;

	materials[0] = Material::get("basic");
	materials[1] = Material::get("deferredBasic");

	const std::string& material_name = get_material_name_for(name);
	materials[2] = material_name!="" ? Material::get(material_name) : Material::get("deferredBasic");

	// generate buffers & objects
	glGenBuffers(1, &vbo);
	glGenBuffers(1, &ebo);
	glGenVertexArrays(1, &vao);

	glBindVertexArray(vao);
	{
		//---- set buffers and upload data
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * vertices.size(), vertices.data(), GL_STATIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint) * faces.size(), faces.data(), GL_STATIC_DRAW);

		//---- set vertex attrib pointsers
		glVertexAttribPointer(
				0, // attrib index
				3, // num of data elems
				GL_FLOAT, // data type
				GL_FALSE, // normalized
				sizeof(Vertex), // stride size
				(void*)offsetof(Vertex, position)); // offset from stride start
		glEnableVertexAttribArray(0);

		glVertexAttribPointer(
				1, // attrib index
				3, // num of data elems
				GL_FLOAT, // data type
				GL_FALSE, // normalized
				sizeof(Vertex), // stride size
				(void*)offsetof(Vertex, normal)); // offset from stride start
		glEnableVertexAttribArray(1);

		glVertexAttribPointer(
				2, // attrib index
				3, // num of data elems
				GL_FLOAT, // data type
				GL_FALSE, // normalized
				sizeof(Vertex), // stride size
				(void*)offsetof(Vertex, tangent)); // offset from stride start
		glEnableVertexAttribArray(2);

		glVertexAttribPointer(
				3, // attrib index
				2, // num of data elems
				GL_FLOAT, // data type
				GL_FALSE, // normalized
				sizeof(Vertex), // stride size
				(void*)offsetof(Vertex, uv)); // offset from stride start
		glEnableVertexAttribArray(3);
	}
	glBindVertexArray(0);

	LOG("loaded mesh '%s' of %lu vertices and %d triangles", name.c_str(), vertices.size(), get_num_triangles());
}

void Mesh::generate_aabb() {
	mat4 o2w = object_to_world();
	aabb = AABB();
	for (int i=0; i<vertices.size(); i++) {
		aabb.add_point(o2w * vec4(vertices[i].position, 1));
	}
}

Mesh::~Mesh() {
	if (bsdf) delete bsdf;
	glDeleteBuffers(1, &vbo);
	glDeleteBuffers(1, &ebo);
	glDeleteVertexArrays(1, &vao);
}

bool Mesh::handle_event(SDL_Event event) {
	return Drawable::handle_event(event);
}

void Mesh::update(float elapsed) {
	Drawable::update(elapsed);
	locked = true;
}

void Mesh::draw() {

	Scene* scene = get_scene();

	// set material
	if (scene->replacement_material) {
		scene->replacement_material->use(this);
	} else {
		materials[scene->material_set]->use(this);
	}

	// bind vao and draw
	glBindVertexArray(vao);
	glDrawElements(GL_TRIANGLES, faces.size(), GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);

	glUseProgram(0);

	// draw children
	Drawable::draw();
}

void Mesh::set_local_position(vec3 _local_position) {
	if (!locked) {
		local_position_value = _local_position;
		generate_aabb();
		get_scene()->generate_aabb();
	}
}

void Mesh::set_rotation(quat _rotation) {
	if (!locked) {
		rotation_value = _rotation;
		generate_aabb();
		get_scene()->generate_aabb();
	}
}

void Mesh::set_scale(vec3 _scale) {
	if (!locked) {
		scale_value = _scale;
		generate_aabb();
		get_scene()->generate_aabb();
	}
}

std::vector<Mesh*> Mesh::LoadMeshes(const std::string& source, bool initialize_graphics) {
	// import mesh from source
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(source,
			aiProcess_GenSmoothNormals |
			aiProcess_CalcTangentSpace |
			aiProcess_Triangulate |
			aiProcess_JoinIdenticalVertices |
			aiProcess_SortByPType);
	if (!scene) {
		ERR("%s", importer.GetErrorString());
	}

	// access and create mesh drawables from imported source
	std::vector<Mesh*> meshes;
	for (int i=0; i<scene->mNumMeshes; i++) {
		aiMesh* mesh = scene->mMeshes[i];
		Mesh* sceneMesh = new Mesh(mesh);
		if (initialize_graphics) sceneMesh->initialize();
		if (mesh) meshes.push_back(sceneMesh);
	}

	LOG("loaded %lu meshe(s)", meshes.size());

	// importer seems to automatically handle memory release for scene
	return meshes;

}
