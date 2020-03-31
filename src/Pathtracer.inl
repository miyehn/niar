#include "Triangle.hpp"
#include "Camera.hpp"

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

vec3 Pathtracer::trace_ray(Ray& ray) {
  bool hit = false;

  for (size_t i = 0; i < triangles.size(); i++) {
    float t; vec3 n;
    hit = hit || triangles[i].intersect(ray, t, n);
  }

  return hit ? vec3(1, 0.5, 0.4) : vec3(0, 0, 0);
}

vec3 Pathtracer::raytrace_pixel(size_t index) {
  std::vector<Ray> rays = generate_rays(index);

  vec3 result = vec3(0);
  for (size_t i = 0; i < rays.size(); i++) {
    result += trace_ray(rays[i]) / float(rays.size());
  }

  return result;
}
