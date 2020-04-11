#include "Primitive.hpp"
#include "Camera.hpp"

#define MAX_RAY_DEPTH 16
#define RAYS_PER_PIXEL 256
#define AREA_LIGHT_SAMPLES 2
#define USE_DIRECT_LIGHT 0

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
    result += trace_ray(rays[i], 0, false) / float(rays.size());
  }

  result = clamp(result, vec3(0), vec3(1));
	return result;
}

// DEBUG ONLY!!!
void Pathtracer::raytrace_debug(size_t index) {
  size_t w = index % width;
  size_t h = index / width;
	Ray ray;
	ray.o = Camera::Active->position;
	ray.tmin = 0.0f;
	ray.tmax = INF;

	float fov = Camera::Active->fov;
	// dx, dy: deviation from canvas center, normalized to range [-1, 1]
	vec2 offset = vec2(0.5f, 0.5f);
	float half_width = float(width) / 2.0f;
	float half_height = float(height) / 2.0f;
	float dx = (w + offset.x - half_width) / half_width;
	float dy = (h + offset.y - half_height) / half_height;
	// the raytraced image plane is at plane z = -1. Supposed k is its size in half.
	float k_y = tan(fov / 2.0f);
	float k_x = k_y * Camera::Active->aspect_ratio;

	vec4 d_cam = vec4(normalize(vec3(k_x * dx, k_y * dy, -1)), 1);
	ray.d = vec3(Camera::Active->camera_to_world_rotation() * d_cam);

	vec3 color = trace_ray(ray, 0, true);
	LOGF("result color: %f %f %f", color.x, color.y, color.z);
	LOG("---------------------");
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

vec3 Pathtracer::trace_ray(Ray& ray, int ray_depth, bool debug) {
	if (ray_depth >= MAX_RAY_DEPTH) return vec3(0);

  // info of closest hit
	Primitive* primitive = nullptr;
  const BSDF* bsdf = nullptr;
  double t; vec3 n;
  for (size_t i = 0; i < primitives.size(); i++) {
    Primitive* prim_tmp = primitives[i]->intersect(ray, t, n, true);
    if (prim_tmp) {
			primitive = prim_tmp;
			bsdf = primitive->bsdf;
		}
  }

  if (primitive) { // intersected with at least 1 primitive (has valid t, n, bsdf)
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

				// Mesh lights
				AreaLight* area_light = dynamic_cast<AreaLight*>(lights[i]);
				if (area_light) {
					for (size_t j = 0; j < AREA_LIGHT_SAMPLES; j++) {

						// get ray to light
						Ray ray_to_light;
						float pdf = area_light->ray_to_light_pdf(ray_to_light, ray.o + float(t)*ray.d);

						// test if ray to light hits anything other than the starting primitive and the light
						double tmp_t; vec3 tmp_n;
						bool in_shadow = false;
						for (size_t j = 0; j < primitives.size(); j++) {
							Primitive* hit_prim = primitives[j]->intersect(ray_to_light, tmp_t, tmp_n, false);
							if (hit_prim && hit_prim!=primitive && hit_prim!=area_light->triangle) in_shadow = true; 
						}

						// add contribution
						if (!in_shadow) {
							vec3 wi = w2h * ray_to_light.d;
							vec3 wo = -w2h * ray.d;
							// NOTE: funny how spheres can technically cast self shadows but this cosine gives the same effect
							// Thanks convexity?
							float costheta_p = std::max(0.0f, dot(n, ray_to_light.d));
							vec3 L_direct = area_light->get_emission() * bsdf->f(wi, wo) * costheta_p / pdf;
							if (isinf(L_direct.x) || isinf(L_direct.y) || isinf(L_direct.z)) L_direct = vec3(0);
							L += L_direct * (1.0f / AREA_LIGHT_SAMPLES);
						}
					}
				}

				// Other types of lights (TODO)

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
			float costheta = abs(dot(n, wi)); 

			if (debug) {
				vec3 hit = ray.o + float(t)*ray.d;
				LOGF("---- hit at depth %d at (%f %f %f) ----", ray_depth, hit.x, hit.y, hit.z);
				LOGF("wo: %f %f %f (normalized to %f %f %f)", 
						-ray.d.x, -ray.d.y, -ray.d.z, wo_hemi.x, wo_hemi.y, wo_hemi.z);
				LOGF("wi: %f %f %f (normalized to %f %f %f)", 
						wi.x, wi.y, wi.z, wi_hemi.x, wi_hemi.y, wi_hemi.z);
			}

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
				vec3 refl_offset = wi_hemi.z > 0 ? EPSILON * n : -EPSILON * n;
				Ray ray_refl(ray.o + float(t)*ray.d + refl_offset, wi); // alright I give up for now...
				// if it has some termination probability, weigh it more if it's not terminated
				Li = trace_ray(ray_refl, ray_depth + 1, debug) * (1.0f / (1.0f - termination_prob));
			}

			L += Li * f * costheta / pdf;
		}
#endif
		if (debug) LOGF("level %d returns: (%f %f %f)", ray_depth, L.x, L.y, L.z);
    return clamp(L, vec3(0), vec3(INF));
  }
  return vec3(0);
}
