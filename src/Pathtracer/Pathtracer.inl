#include "Triangle.hpp"
#include "Camera.hpp"

#define MAX_RAY_DEPTH 2
#define RAYS_PER_PIXEL 1

#define RR_THRESHOLD 0.2f

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

  result = clamp(result, vec3(0), vec3(1));
	return result;
}

void Pathtracer::raytrace_tile(size_t X, size_t Y) {
	LOGF("tracing tile %d, %d", X, Y);
	
	size_t tile_w = std::min(tile_size, width - X * tile_size);
	size_t tile_h = std::min(tile_size, height - Y * tile_size);

	size_t x_offset = X * tile_size;
	size_t y_offset = Y * tile_size;

	for (size_t y = 0; y < tile_h; y++) {
		for (size_t x = 0; x < tile_w; x++) {

			size_t px_index_main = width * (y_offset + y) + (x_offset + x);
			vec3 color = raytrace_pixel(px_index_main);
			set_mainbuffer_rgb(px_index_main, color);

			size_t px_index_sub = y * tile_w + x;
			set_subbuffer_rgb(0, px_index_sub, color);

		}
	}

	upload_tile(0, x_offset, y_offset, tile_w, tile_h);
}

mat3 make_h2w(const vec3& n) { // TODO: make more robust
	vec3 z = n;
	// choose a vector different from z
	vec3 tmp = vec3(n.z, n.x, n.y);
	
	vec3 x = cross(tmp, z);
	x = normalize(x);
	vec3 y = cross(z, x);

	return mat3(x, y, z);
}

// see: https://stackoverflow.com/questions/687261/converting-rgb-to-grayscale-intensity
float brightness(vec3 color) {
	return 0.2989f * color.r + 0.587f * color.g + 0.114 * color.b;
}

vec3 Pathtracer::trace_ray(Ray& ray, int ray_depth) {
	if (ray_depth >= MAX_RAY_DEPTH) return vec3(0);

  // info of closest hit
  const BSDF* bsdf = nullptr;
  float t; vec3 n;
  for (size_t i = 0; i < triangles.size(); i++) {
    const BSDF* bsdf_tmp = triangles[i]->intersect(ray, t, n);
    if (bsdf_tmp) bsdf = bsdf_tmp;
  }

  if (bsdf) { // intersected with at least 1 triangle (has valid t, n, bsdf)
    vec3 L = vec3(0);

    //---- emission ----
    L += bsdf->Le;

		// construct transform from hemisphere space to world space;
		// used by both direct and indirect lighting
		vec3 axis = cross(vec3(0, 0, 1), n);
		mat3 h2w = make_h2w(n);
		mat3 w2h = transpose(h2w);

		//---- direct light contribution ----
#if 1
		if (!bsdf->is_delta) {
			for (size_t i = 0; i < lights.size(); i++) {
				Ray ray_to_light;
				float pdf = lights[i]->ray_to_light_pdf(ray_to_light, ray.o + ray.d * t + n * EPSILON);

				float to_light_t; vec3 light_n;
				bool in_shadow = false;
				for (size_t j = 0; j < triangles.size(); j++) {
					if (triangles[j]->intersect(ray_to_light, to_light_t, light_n)) in_shadow = true; 
				}
				if (!in_shadow) {
					vec3 wi = w2h * ray_to_light.d;
					vec3 wo = -w2h * ray.d;
					float costheta = abs(dot(n, -ray_to_light.d));
					vec3 L_direct = lights[i]->get_emission() * bsdf->f(wi, wo) * costheta / pdf;
					L += L_direct;
				}

			}
		}
#endif

#if 1
    //---- scatter (recursive part) ----
    vec3 wi_hemi; // assigned by pdf in hemisphere space
		vec3 wo_hemi = w2h * (-ray.d);
		float pdf;
    vec3 f = bsdf->sample_f(pdf, wi_hemi, wo_hemi);

		// transform wi back to world space
		vec3 wi = h2w * wi_hemi;
    float costheta = abs(dot(n, wi)); // [0, 1], not considering front/back face here

		// russian roulette
		float termination_prob = 0.0f;
		ray.contribution *= brightness(f) * costheta;
		if (ray.contribution < RR_THRESHOLD) {
			termination_prob = (RR_THRESHOLD - ray.contribution) / RR_THRESHOLD;
		}
		bool terminate = sample::rand01() < termination_prob;

		// recursive step: trace scattered ray in wi direction (if not terminated by RR)
		vec3 Li = vec3(0);
		if (!terminate) {
			Ray ray_refl(ray.o + t * ray.d + n * EPSILON, wi);
			// if it has some termination probability, weigh it more if it's not terminated
			Li = trace_ray(ray_refl, ray_depth + 1) * (1.0f / (1.0f - termination_prob));
		}

    L += Li * f * costheta / pdf;
#endif

    return L;
  }
  return vec3(0);
}
