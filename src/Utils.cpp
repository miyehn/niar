#include "Utils.hpp"
#include "Scene.hpp"
#include "Mesh.hpp"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wall"
#include "openfbx/ofbx.h"
#pragma GCC diagnostic pop

//------------------ AABB -------------------------

void AABB::add_point(vec3 p) {
	min.x = glm::min(min.x, p.x);
	min.y = glm::min(min.y, p.y);
	min.z = glm::min(min.z, p.z);
	max.x = glm::max(max.x, p.x);
	max.y = glm::max(max.y, p.y);
	max.z = glm::max(max.z, p.z);
}

void AABB::merge(const AABB& other) {
	min.x = glm::min(min.x, other.min.x);
	min.y = glm::min(min.y, other.min.y);
	min.z = glm::min(min.z, other.min.z);
	max.x = glm::max(max.x, other.max.x);
	max.y = glm::max(max.y, other.max.y);
	max.z = glm::max(max.z, other.max.z);
}

AABB AABB::merge(const AABB& A, const AABB& B) {
	AABB res;
	res.merge(A);
	res.merge(B);
	return res;
}

std::vector<vec3> AABB::corners() {
	std::vector<vec3> res(8);
	res[0] = min;
	res[1] = vec3(max.x, min.y, min.z);
	res[2] = vec3(min.x, max.y, min.z);
	res[3] = vec3(max.x, max.y, min.z);
	res[4] = vec3(min.x, min.y, max.z);
	res[5] = vec3(max.x, min.y, max.z);
	res[6] = vec3(min.x, max.y, max.z);
	res[7] = max;
	return res;
}

//-------------------------------------------------

ofbx::IScene* load_scene(const char* path) {
	FILE* fp = fopen(path, "rb");
	if (!fp) {
		ERRF("cannot open file: %s", path);
		return nullptr;
	}
	fseek(fp, 0, SEEK_END);
	long file_size = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	auto* content = new ofbx::u8[file_size];
	fread(content, 1, file_size, fp);
	ofbx::IScene* inScene = ofbx::load(
			(ofbx::u8*)content, 
			file_size, 
			(ofbx::u64)ofbx::LoadFlags::TRIANGULATE);
	if (!inScene) {
		ERRF("error importing scene: %s", ofbx::getError());
		return nullptr;
	}
	fclose(fp);
	return inScene;
}

// see openfbx example for file handling
Scene* f::import_scene(const char* path) {

	ofbx::IScene* inScene = load_scene(path);
	if (!inScene) return nullptr;

	int mesh_count = inScene->getMeshCount();
	Scene* scene = new Scene(path);
	for (int i=0; i<mesh_count; i++) {
		Mesh* mesh = new Mesh();
		const ofbx::Mesh* inMesh = inScene->getMesh(i);
		const ofbx::Geometry* geom = inMesh->getGeometry();
		int vertex_count = geom->getVertexCount();
		const ofbx::Vec3* vertices = geom->getVertices();
		for (int j=0; j<vertex_count; j++) {
			ofbx::Vec3 v = vertices[j];

		}
		mesh->initialize();
	}

	return scene;
}

//-------------------------------------------------

quat quat_from_dir(vec3 dir) {
	if (dot(dir, vec3(0, 0, -1)) > 1.0f - EPSILON) {
		return quat();
	}
	float costheta = dot(dir, vec3(0, 0, -1));
	float angle = acos(dot(dir, vec3(0, 0, -1)));
	vec3 axis = normalize(cross(vec3(0, 0, -1), dir));

	float c = cos(angle / 2);
	float s = sin(angle / 2);

	return quat(c, s*axis.x, s*axis.y, s*axis.z);
}

std::string s3(vec3 v) { 
	return ("(" + std::to_string(v.x) + ", " + std::to_string(v.y) + ", "  + std::to_string(v.z) + ")").c_str();
}

std::string lower(const std::string& s) {
	std::string res = "";
	std::locale loc;
	for(int i=0; i<s.length(); i++) {
#ifdef WIN32
		res += std::tolower(s[i]);
#else
		res += std::tolower(s[i], loc);
#endif
	}
	return res;
}

