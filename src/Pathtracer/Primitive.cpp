#include "Primitive.hpp"
#include "Mesh.hpp"
#include "BSDF.hpp"

#define USE_EMBEDDED_NORMAL 0
#define USE_INTERPOLATED_NORMAL 0

Triangle::Triangle(
		const mat4& o2w, 
		const Vertex& v1, 
		const Vertex& v2, 
		const Vertex& v3,
		BSDF* _bsdf) {
  // position
  vertices[0] = vec3(o2w * vec4(v1.position, 1));
  vertices[1] = vec3(o2w * vec4(v2.position, 1));
  vertices[2] = vec3(o2w * vec4(v3.position, 1));

  // precompute true normal and distance to origin
  vec3 u = normalize(vertices[1] - vertices[0]);
  vec3 v = normalize(vertices[2] - vertices[0]);
  if (dot(u, v) > 1.0f - 0.1f * EPS_F) { // if unfortunately picked a very sharp angle
    u = normalize(vertices[2] - vertices[1]);
    v = normalize(vertices[0] - vertices[1]);
  }
  plane_n = normalize(cross(u, v));
  plane_k = dot(vertices[0], plane_n);

// normal. TODO: correct to transform this way even if object is non-uniformly scaled?
#if USE_EMBEDDED_NORMAL
  normals[0] = normalize(vec3(o2w * vec4(v1.normal, 1)));
  normals[1] = normalize(vec3(o2w * vec4(v2.normal, 1)));
  normals[2] = normalize(vec3(o2w * vec4(v3.normal, 1)));
#else
  normals[0] = plane_n;
  normals[1] = plane_n;
  normals[2] = plane_n;
#endif

	// precompute area
	vec3 e1 = vertices[1] - vertices[0];
	vec3 e2 = vertices[2] - vertices[0];
	area = length(cross(e1, e2)) * 0.5f;

  // precompute edge normals
  enormals[0] = normalize(cross(plane_n, vertices[1] - vertices[0]));
  enormals[1] = normalize(cross(plane_n, vertices[2] - vertices[1]));
  enormals[2] = normalize(cross(plane_n, vertices[0] - vertices[2]));

  bsdf = _bsdf;
}

/* (o + t*d) . n = k
 * o . n + t*d . n = k
 * t * (d . n) = k - o . n
 * t = (k - o . n) / (d . n)
 */
const BSDF* Triangle::intersect(Ray& ray, float& t, vec3& normal) const {
  // ray parallel to plane
  float d_dot_n = dot(ray.d, plane_n);
  if (abs(d_dot_n) == 0.0f) return nullptr;
  // intersection out of range
  float _t = (plane_k - dot(ray.o, plane_n)) / d_dot_n;
  if (_t < ray.tmin || _t > ray.tmax) return nullptr;

  vec3 p = ray.o + _t * ray.d;
  // barycentric coordinate with axes v[1] - v[0], v[2] - v[0]
  vec3 p0 = p - vertices[0];
#if USE_INTERPOLATED_NORMAL // barycentric coords
  // also see: https://gamedev.stackexchange.com/questions/23743/whats-the-most-efficient-way-to-find-barycentric-coordinates
  float u = dot(p0, enormals[0]) / dot(vertices[2] - vertices[0], enormals[0]);
  float v = dot(p0, enormals[2]) / dot(vertices[1] - vertices[0], enormals[2]);
  if (u < 0 || v < 0 || u + v > 1) return nullptr;
#else // test sides for each edge. Gives wrong uv though.
  // other early outs: intersection not in triangle TODO: precompute some of these
  if (dot(p - vertices[0], enormals[0]) < 0) return nullptr;
  if (dot(p - vertices[1], enormals[1]) < 0) return nullptr;
  if (dot(p - vertices[2], enormals[2]) < 0) return nullptr;
#endif

  // intersection is valid.
  ray.tmax = _t;
  t = _t;
#if USE_INTERPOLATED_NORMAL
  normal = normalize(normals[0] + u * normals[1] + v * normals[2]);
#else
  normal = plane_n;
#endif
  return bsdf;
}

vec3 Triangle::sample_point() const {
	float u = sample::rand01();
	float v = sample::rand01();
	if (u + v > 1) {
		u = 1.0f - u;
		v = 1.0f - v;
	}
	
	vec3 e1 = vertices[1] - vertices[0];
	vec3 e2 = vertices[2] - vertices[0];

	return vertices[0] + e1 * u + e2 * v;
}

//-------------- Sphere ----------------

Sphere::~Sphere() {
	delete bsdf;
}

const BSDF* Sphere::intersect(Ray& ray, float& t, vec3& normal) const {
	vec3 p = ray.o - center;
	float r2 = r * r;
	// a = 1
	float b = 2 * dot(p, ray.d);
	float c = dot(p, p) - r2;
	float delta = b * b - 4 * c;

	if (delta < 0) return nullptr; // no intersection

	float rt_delta = sqrt(delta);
	float t_tmp = 0.5f * (-b - rt_delta); // t1
	if (t_tmp < ray.tmin || t_tmp > ray.tmax) t_tmp = 0.5f * (-b + rt_delta); // t2
	if (t_tmp < ray.tmin || t_tmp > ray.tmax) return nullptr; // intersection(s) not valid

	// outputs
	t = t_tmp;
	ray.tmax = t_tmp;
	normal = normalize(ray.o + t * ray.d - center);
	return bsdf;
}
