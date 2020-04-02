#include "Triangle.hpp"
#include "Camera.hpp"

#define MAX_RAY_DEPTH 2

std::vector<Ray> Pathtracer::generate_rays(size_t index) {
  size_t w = index % width;
  size_t h = index / width;

  std::vector<Ray> rays;

  Ray ray;
  ray.o = Camera::Active->position;
  ray.tmin = 0.0f;
  ray.tmax = INF;

  float fov = Camera::Active->fov;
  // dx, dy: deviation from canvas center, normalized to range [-1, 1]
  vec2 offset = vec2(0.5f);
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
  return rays;

}

vec3 Pathtracer::raytrace_pixel(size_t index) {
  std::vector<Ray> rays = generate_rays(index);

  vec3 result = vec3(0);
  for (size_t i = 0; i < rays.size(); i++) {
    result += trace_ray(rays[i], 0) / float(rays.size());
  }

  return result;
}

vec3 Pathtracer::trace_ray(Ray& ray, int ray_depth) {
  // info of closest hit
  const BSDF* bsdf = nullptr;
  float t; vec3 n;
  for (size_t i = 0; i < triangles.size(); i++) {
    const BSDF* bsdf_tmp = triangles[i].intersect(ray, t, n);
    if (bsdf_tmp) bsdf = bsdf_tmp;
  }
  if (bsdf) { // intersected with at least 1 triangle (has valid t, n, bsdf)
    
#if 1
    return vec3(1, 1, 1);
#endif
    
    vec3 L = vec3(0);
    // emission
    L += bsdf->Le;
    // scatter
    vec3 wi;
    float pdf = bsdf->f(wi, -ray.d, n);
    Ray ray_refl(ray.o + t * ray.d, wi);
    vec3 L_indirect = ray_depth >= MAX_RAY_DEPTH ? 
      vec3(0) :
      trace_ray(ray_refl, ray_depth + 1);

    float costheta = abs(dot(n, -ray.d)); // [0, 1], not considering front/back face here
    L += L_indirect * bsdf->albedo * costheta / pdf;

    return L;
  }
  return vec3(0);//hit ? vec3(1, 0.5, 0.4) : vec3(0, 0, 0);
}
