#pragma once
#include "lib.h"
#include "BSDF.hpp"

struct Vertex;
struct BSDF;

struct Ray {
  Ray(vec3 _o = vec3(0), vec3 _d = vec3(0, 0, 1)) : o(_o), d(_d) {
    tmin = 0.0f;
    tmax = INF;
  }
  vec3 o, d;
  float tmin, tmax; // TODO: why need tmin?
};

struct Triangle {

  Triangle(const mat4& o2w, const Vertex& v1, const Vertex& v2, const Vertex& v3, BSDF* _bsdf);
  ~Triangle();

  vec3 vertices[3];
  vec3 normals[3];
  vec3 enormals[3]; // cached edge normals

  // other pre-computed values
  vec3 plane_n;
  float plane_k;
	float area;

	// this gets passed in from mesh, and will be cleaned up by mesh as well.
  const BSDF* bsdf;

  const BSDF* intersect(Ray& ray, float& t, vec3& normal) const;

	vec3 sample_point() const;

};
