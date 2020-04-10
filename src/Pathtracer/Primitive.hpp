#pragma once
#include "lib.h"

struct Vertex;
struct BSDF;

struct Ray {
  Ray(vec3 _o = vec3(0), vec3 _d = vec3(0, 0, 1)) : o(_o), d(_d) {
    tmin = 0.0f;
    tmax = INF;
		contribution = 1.0f;
  }
  vec3 o, d;
  float tmin, tmax, contribution; // TODO: why need tmin?
};

struct Primitive {
  virtual const BSDF* intersect(Ray& ray, float& t, vec3& normal) const = 0;
	virtual ~Primitive(){}
	const BSDF* bsdf;
};

struct Triangle : public Primitive {

	// bsdf gets passed in from mesh, and will be cleaned up by mesh as well.
  Triangle(const mat4& o2w, const Vertex& v1, const Vertex& v2, const Vertex& v3, BSDF* _bsdf);

  vec3 vertices[3];
  vec3 normals[3];
  vec3 enormals[3]; // cached edge normals

  // other pre-computed values
  vec3 plane_n;
  float plane_k;
	float area;

  const BSDF* intersect(Ray& ray, float& t, vec3& normal) const;

	vec3 sample_point() const;

};

struct Sphere : public Primitive {

	// since spheres are created and used for pathtracer only, 
	// it's responsible for managing bsdf memory as well.
	Sphere(const vec3& _c, float _r, BSDF* _bsdf) : center(_c), r(_r) {
		bsdf = _bsdf;
	}
	~Sphere();

	vec3 center;
	float r;

  const BSDF* intersect(Ray& ray, float& t, vec3& normal) const;
};