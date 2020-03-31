#pragma once
#include "lib.h"

struct Vertex;

struct Ray {
  vec3 o, d;
  float tmin, tmax;
};

struct Triangle {

  Triangle(const Vertex& v1, const Vertex& v2, const Vertex& v3);

  vec3 vertices[3];
  vec3 normals[3];
  vec3 enormals[3]; // cached edge normals

  // pre-computed to allow faster intersection test
  vec3 plane_n;
  float plane_k;

  // TODO: bsdf

  bool intersect(Ray& ray, float& t, vec3& normal);

};
