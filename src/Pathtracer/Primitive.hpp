#pragma once
#include "Utils/myn/Misc.h"

struct Vertex;
struct BSDF;

struct Ray {
	explicit Ray(glm::vec3 _o = glm::vec3(0), glm::vec3 _d = glm::vec3(0, 0, 1)) : o(_o), d(_d) {
		tmin = 0.0f;
		tmax = INF;
		rr_contribution = 1.0f;
	}
	glm::vec3 o, d;
	double tmin, tmax; 
	float rr_contribution; // TODO: why need tmin?
	bool receive_le = false;
};

struct RayTask {
	RayTask() {
		output = glm::vec3(0.0f);
		contribution = glm::vec3(1.0f);
	}
	Ray ray;
	glm::vec3 output{};
	glm::vec3 contribution{};
};

struct Primitive {
	virtual Primitive* intersect(Ray& ray, double& t, glm::vec3& normal, bool modify_ray = true) = 0;
	virtual ~Primitive()= default;
	const BSDF* bsdf{};
};

struct Triangle : public Primitive {

	// bsdf gets passed in from mesh, and will be cleaned up by mesh as well.
	Triangle(const glm::mat4& o2w, const Vertex& v1, const Vertex& v2, const Vertex& v3, BSDF* _bsdf);

	glm::vec3 vertices[3];
	glm::vec3 normals[3];
	glm::vec3 enormals[3]; // cached edge normals

	// other pre-computed values
	glm::vec3 e1, e2;
	glm::vec3 plane_n;
	float plane_k;
	float area;

	Primitive* intersect(Ray& ray, double& t, glm::vec3& normal, bool modify_ray) override;

	glm::vec3 sample_point() const;

};

struct Sphere : public Primitive {

	// since spheres are created and used for pathtracer only, 
	// it's responsible for managing bsdf memory as well.
	Sphere(const glm::vec3& _c, float _r, BSDF* _bsdf) : center(_c), r(_r) {
		bsdf = _bsdf;
	}
	~Sphere() override;

	glm::vec3 center;
	float r;

	Primitive* intersect(Ray& ray, double& t, glm::vec3& normal, bool modify_ray) override;
};
