#include "Primitive.hpp"
#include "Camera.hpp"

#define MAX_RAY_DEPTH 8
#define RAYS_PER_PIXEL 32
#define AREA_LIGHT_SAMPLES 2
#define USE_DIRECT_LIGHT 1

#define RR_THRESHOLD 0.08f

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

mat3 make_h2w(const vec3& n) { // TODO: make more robust
	vec3 z = n;
	// choose a vector different from z
	vec3 tmp = normalize(vec3(1, 2, 3));
	
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
  for (size_t i = 0; i < primitives.size(); i++) {
    const BSDF* bsdf_tmp = primitives[i]->intersect(ray, t, n);
    if (bsdf_tmp) bsdf = bsdf_tmp;
  }

  if (bsdf) { // intersected with at least 1 triangle (has valid t, n, bsdf)
    vec3 L = vec3(0);

		//---- emission ----
#if USE_DIRECT_LIGHT
		// totally a hack...
		if (!bsdf->is_emissive()) L += bsdf->Le;
#else
		L += bsdf->Le;
#endif

		// construct transform from hemisphere space to world space;
		// used by both direct and indirect lighting
		mat3 h2w = make_h2w(n);
		mat3 w2h = transpose(h2w);

#if USE_DIRECT_LIGHT
		//---- direct light contribution ----
		if (!bsdf->is_delta) {
			for (size_t i = 0; i < lights.size(); i++) {

				if (lights[i]->type == Light::Area) {
					for (size_t j = 0; j < AREA_LIGHT_SAMPLES; j++) {
						Ray ray_to_light;
						float pdf = lights[i]->ray_to_light_pdf(ray_to_light, ray.o + ray.d * (t-EPSILON));

						float tmp_t; vec3 tmp_n;
						bool in_shadow = false;
						for (size_t j = 0; j < primitives.size(); j++) {
							if (primitives[j]->intersect(ray_to_light, tmp_t, tmp_n)) in_shadow = true; 
						}
						if (!in_shadow) {
							vec3 wi = w2h * ray_to_light.d;
							vec3 wo = -w2h * ray.d;

							float costheta_p = std::max(0.0f, dot(n, ray_to_light.d));
							vec3 L_direct = pdf <= EPSILON ? 
								vec3(0) : lights[i]->get_emission() * bsdf->f(wi, wo) * costheta_p/ pdf;
							L += L_direct * (1.0f / AREA_LIGHT_SAMPLES);
						}
					}
				}

			}
		}
#endif

#if 1

#if USE_DIRECT_LIGHT
		if (!bsdf->is_emissive()) {
#else
		if (1) {
#endif
			//---- scatter (recursive part) ----
			vec3 wi_hemi; // assigned by pdf in hemisphere space
			vec3 wo_hemi = w2h * (-ray.d);
			float pdf;// = float(bsdf->is_delta && ray_depth == 0);
			vec3 f = bsdf->sample_f(pdf, wi_hemi, wo_hemi);

			// transform wi back to world space
			vec3 wi = h2w * wi_hemi;
			float costheta = std::max(0.0f, dot(n, wi)); 

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
				vec3 refl_o_offset = wi * EPSILON;//dot(wi,n)>0 ? n * EPSILON : -n * EPSILON; // offset to the side of wi
				Ray ray_refl(ray.o + t*ray.d + refl_o_offset, wi);
				// if it has some termination probability, weigh it more if it's not terminated
				Li = trace_ray(ray_refl, ray_depth + 1) * (1.0f / (1.0f - termination_prob));
			}

			L += Li * f * costheta / pdf;
		}
#endif
    return clamp(L, vec3(0), vec3(INF));
  }
  return vec3(0);
}
