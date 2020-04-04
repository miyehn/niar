#include "Triangle.hpp"
#include "Camera.hpp"

#define MAX_RAY_DEPTH 2
#define RAYS_PER_PIXEL 1

std::vector<Ray> Pathtracer::generate_rays(size_t index) {
  size_t w = index % width;
  size_t h = index / width;

  std::vector<Ray> rays;

	Ray ray;
	for (int i = 0; i < RAYS_PER_PIXEL; i++) {
		ray.o = Camera::Active->position;
		ray.tmin = 0.0f;
		ray.tmax = INF;

		float fov = Camera::Active->fov;
		// dx, dy: deviation from canvas center, normalized to range [-1, 1]
		vec2 offset = sample::unit_square_uniform();
		float half_width = float(width) / 2.0f;
		float half_height = float(height) / 2.0f;
		float dx = (w + offset.x - half_width) / half_width;
		float dy = (h + offset.y - half_height) / half_height;
		// the raytraced image plane is at plane z = -1. Supposed k is its size in half.
		float k_y = tan(fov / 2.0f);
		float k_x = k_y * Camera::Active->aspect_ratio;

		vec4 d_cam = vec4(normalize(vec3(k_x * dx, k_y * dy, -1)), 1);
		ray.d = vec3(Camera::Active->camera_to_world_rotation() * d_cam);

		rays.push_back(ray);
	}
  return rays;

}

vec3 Pathtracer::raytrace_pixel(size_t index) {
  std::vector<Ray> rays = generate_rays(index);

  vec3 result = vec3(0);
  for (size_t i = 0; i < rays.size(); i++) {
    result += trace_ray(rays[i], 0) / float(rays.size());
  }

  return clamp(result, vec3(0), vec3(1));
}

vec3 Pathtracer::trace_shadow_ray(Light* light, const vec3& origin) {
  float t; vec3 n; Ray ray;
	float pdf = light->ray_to_light(ray, origin);
  for (size_t i = 0; i < triangles.size(); i++) {
		if (triangles[i].intersect(ray, t, n)) return vec3(0);
	}
	return vec3(1);
}

vec3 Pathtracer::trace_ray(Ray& ray, int ray_depth) {
	if (ray_depth >= MAX_RAY_DEPTH) return vec3(0);

  // info of closest hit
  const BSDF* bsdf = nullptr;
  float t; vec3 n;
  for (size_t i = 0; i < triangles.size(); i++) {
    const BSDF* bsdf_tmp = triangles[i].intersect(ray, t, n);
    if (bsdf_tmp) bsdf = bsdf_tmp;
  }

  if (bsdf) { // intersected with at least 1 triangle (has valid t, n, bsdf)
    vec3 L = vec3(0);

    //---- emission ----
    L += bsdf->Le;

		//---- direct light contribution ----
#if 1
		for (size_t i = 0; i < lights.size(); i++) {
			vec3 L_direct = trace_shadow_ray(lights[i], ray.o + ray.d * t + n * EPSILON);
			L += L_direct;
			//L += L_direct * bsdf->albedo * abs(dot(n, to_light.d));
		}
#endif

    //---- scatter (recursive part) ----
		// construct transform from hemisphere space to world space
		vec3 axis = cross(vec3(0, 0, 1), n);
		mat4 hemi_to_world = mat4(1);
		if (dot(axis, axis) > EPSILON) {
			float angle = acos(n.z);
			hemi_to_world = rotate(mat4(1), angle, axis);
		}
    vec3 wi; // assigned by f in hemisphere space
    float pdf = bsdf->pdf(wi, -ray.d);
		// transform wi back to world space
		wi = vec3(hemi_to_world * vec4(wi, 1));

		// recursive step: trace scattered ray in wi direction
    Ray ray_refl(ray.o + t * ray.d + n * EPSILON, wi);
    vec3 L_indirect = trace_ray(ray_refl, ray_depth + 1);

    float costheta = abs(dot(n, wi)); // [0, 1], not considering front/back face here
    L += L_indirect * bsdf->f(wi, ray.d) * costheta / pdf;

    return L;
  }
  return vec3(0);//hit ? vec3(1, 0.5, 0.4) : vec3(0, 0, 0);
}
