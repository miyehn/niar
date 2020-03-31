#include "Triangle.hpp"
#include "Mesh.hpp"

#define EPSILON 0.00001f
#define USE_INTERPOLATED_NORMAL 0

Triangle::Triangle(const Vertex& v1, const Vertex& v2, const Vertex& v3) {
  // position
  vertices[0] = v1.position;
  vertices[1] = v2.position;
  vertices[2] = v3.position;
  // normal
  normals[0] = v1.normal;
  normals[1] = v2.normal;
  normals[2] = v3.normal;

  // precompute true normal and distance to origin
  vec3 u = normalize(vertices[1] - vertices[0]);
  vec3 v = normalize(vertices[2] - vertices[1]);
  if (dot(u, v) > 1.0f - EPSILON) { // if unfortunately picked a very sharp angle
    u = v;
    v = normalize(vertices[0] - vertices[2]);
  }
  plane_n = normalize(cross(u, v));
  plane_k = dot(vertices[0], plane_n);

  // precompute edge normals
  enormals[0] = normalize(cross(plane_n, vertices[1] - vertices[0]));
  enormals[1] = normalize(cross(plane_n, vertices[2] - vertices[1]));
  enormals[2] = normalize(cross(plane_n, vertices[0] - vertices[2]));
}

/* (o + t*d) . n = k
 * o . n + t*d . n = k
 * t * (d . n) = k - o . n
 * t = (k - o . n) / (d . n)
 */
bool Triangle::intersect(Ray& ray, float& t, vec3& normal) {
  // ray parallel to plane
  float d_dot_n = dot(ray.d, plane_n);
  if (abs(d_dot_n) < EPSILON) return false;
  // intersection out of range
  float _t = (plane_k - dot(ray.o, plane_n)) / d_dot_n;
  if (_t < ray.tmin || _t > ray.tmax) return false;

  vec3 p = ray.o + _t * ray.d;
  // barycentric coordinate with axes v[1] - v[0], v[2] - v[0]
  vec3 p0 = p - vertices[0];
#if USE_INTERPOLATED_NORMAL // barycentric coords
  // also see: https://gamedev.stackexchange.com/questions/23743/whats-the-most-efficient-way-to-find-barycentric-coordinates
  float u = dot(p0, enormals[0]) / dot(vertices[2] - vertices[0], enormals[0]);
  float v = dot(p0, enormals[2]) / dot(vertices[1] - vertices[0], enormals[2]);
  if (u < 0 || v < 0 || u + v > 1) return false;
#else // test sides for each edge. Gives wrong uv though.
  vec3 edge;
  // other early outs: intersection not in triangle TODO: precompute some of these
  edge = vertices[1] - vertices[0];
  if (dot(p - vertices[0], enormals[0]) < 0) return false;
  edge = vertices[2] - vertices[1];
  if (dot(p - vertices[1], enormals[1]) < 0) return false;
  edge = vertices[0] - vertices[2];
  if (dot(p - vertices[2], enormals[2]) < 0) return false;
#endif

  // intersection is valid.
  ray.tmin = _t;
  t = _t;
#if USE_INTERPOLATED_NORMAL
  normal = normalize(normals[0] + u * normals[1] + v * normals[2]);
#else
  normal = plane_n;
#endif
  return true;
}
