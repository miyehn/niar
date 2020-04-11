#include "Primitive.hpp"
#include "Camera.hpp"

#define MAX_RAY_DEPTH 16
#define RAYS_PER_PIXEL 4 // for profile: 4
#define AREA_LIGHT_SAMPLES 2
#define USE_DIRECT_LIGHT 1

#define RR_THRESHOLD 0.05f

void Pathtracer::generate_rays(std::vector<Ray>& rays, size_t index) {
	rays.clear();

  size_t w = index % width;
  size_t h = index / width;
	float fov = Camera::Active->fov;
	float half_width = float(width) / 2.0f;
	float half_height = float(height) / 2.0f;
	// the raytraced image plane is at plane z = -1. Supposed k is its size in half.
	float k_y = tan(fov / 2.0f);
	float k_x = k_y * Camera::Active->aspect_ratio;

	Ray ray;
	ray.o = Camera::Active->position;
	ray.tmin = 0.0;
	ray.tmax = INF;

	for (int i = 0; i < RAYS_PER_PIXEL; i++) {
		// dx, dy: deviation from canvas center, normalized to range [-1, 1]
		vec2 offset = sample::unit_square_uniform();
		float dx = (w + offset.x - half_width) / half_width;
		float dy = (h + offset.y - half_height) / half_height;

		vec4 d_cam = vec4(normalize(vec3(k_x * dx, k_y * dy, -1)), 1);
		ray.d = vec3(Camera::Active->camera_to_world_rotation() * d_cam);

		rays.push_back(ray);
	}
}

vec3 Pathtracer::raytrace_pixel(size_t index) {
  std::vector<Ray> rays;
	generate_rays(rays, index);

  vec3 result = vec3(0);
  for (size_t i = 0; i < rays.size(); i++) {
    result += trace_ray(rays[i], 0, false);
  }

	result *= 1.0f / float(rays.size());
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

void make_h2w(mat3& h2w, const vec3& z) { // TODO: make more robust
	// choose a vector different from z
	vec3 tmp = normalize(vec3(1, 2, 3));
	
	vec3 x = cross(tmp, z);
	x = normalize(x);
	vec3 y = cross(z, x);

	h2w = mat3(x, y, z);
}

// see: https://stackoverflow.com/questions/687261/converting-rgb-to-grayscale-intensity
float brightness(const vec3& color) {
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

		// pre-compute (or declare) some common things to be used later
    vec3 L = vec3(0);
		vec3 hit_p = ray.o + float(t) * ray.d;
		// construct transform from hemisphere space to world space;
		mat3 h2w; 
		make_h2w(h2w, n);
		mat3 w2h = transpose(h2w);
		// wi, wo
		vec3 wo_world = -ray.d;
		vec3 wo_hemi = -w2h * ray.d;
		// 
		vec3 wi_world; // to be transformed from wi_hemi
		vec3 wi_hemi; // to be assigned by f
		float costhetai; // some variation of dot(wi_world, n)

		//---- emission ----
#if USE_DIRECT_LIGHT
		// totally a hack...
		if (ray_depth == 0 || ray.receive_le || !bsdf->is_emissive) L += bsdf->get_emission();
#else
		L += bsdf->get_emission();
#endif


#if 1//USE_DIRECT_LIGHT
		//---- direct light contribution ----
		if (!bsdf->is_delta) {
			for (size_t i = 0; i < lights.size(); i++) {

				// Mesh lights
				AreaLight* area_light = dynamic_cast<AreaLight*>(lights[i]);
				if (area_light) {
					float each_sample_weight = 1.0f / AREA_LIGHT_SAMPLES;
					for (size_t j = 0; j < AREA_LIGHT_SAMPLES; j++) {

						// get ray to light
						Ray ray_to_light;
						// in this case not a real pdf, but just something to divide by?
						float pdf = area_light->ray_to_light_pdf(ray_to_light, hit_p);

						// test if ray to light hits anything other than the starting primitive and the light
						double tmp_t; vec3 tmp_n;
						bool in_shadow = false;
						for (size_t j = 0; j < primitives.size(); j++) {
							Primitive* hit_prim = primitives[j]->intersect(ray_to_light, tmp_t, tmp_n, false);
							if (hit_prim && hit_prim!=primitive && hit_prim!=area_light->triangle) {
								in_shadow = true;
								break;
							}
						}

						// add contribution
						if (!in_shadow) {
							wi_world = ray_to_light.d;
							wi_hemi = w2h * wi_world;
							// funny how spheres can technically cast self shadows but clampint cosine gives same effect
							// thanks convexity?
							costhetai = std::max(0.0f, dot(n, wi_world));
							vec3 L_direct = area_light->get_emission() * bsdf->f(wi_hemi, wo_hemi) * costhetai / pdf;
							// correction for when above num and denom both 0. TODO: is this right?
							if (isnan(L_direct.x) || isnan(L_direct.y) || isnan(L_direct.z)) L_direct = vec3(0);
							L += L_direct * each_sample_weight;
						}
					}
				}

				// Other types of lights (TODO)

			}
		}
#endif

#if 1 // indirect lighting (recursive)

#if USE_DIRECT_LIGHT
		if (!bsdf->is_emissive) {
#else
		if (1) {
#endif
			float pdf;
			vec3 f = bsdf->sample_f(pdf, wi_hemi, wo_hemi);

			// transform wi back to world space
			wi_world = h2w * wi_hemi;
			costhetai = abs(dot(n, wi_world)); 

			if (debug) {
				LOGF("---- hit at depth %d at (%f %f %f) ----", ray_depth, hit_p.x, hit_p.y, hit_p.z);
				LOGF("wo: %f %f %f (normalized to %f %f %f)", 
						wo_world.x, wo_world.y, wo_world.z, wo_hemi.x, wo_hemi.y, wo_hemi.z);
				LOGF("wi: %f %f %f (normalized to %f %f %f)", 
						wi_world.x, wi_world.y, wi_world.z, wi_hemi.x, wi_hemi.y, wi_hemi.z);
			}

			// russian roulette
			float termination_prob = 0.0f;
			ray.contribution *= brightness(f) * costhetai;
			if (ray.contribution < RR_THRESHOLD) {
				termination_prob = (RR_THRESHOLD - ray.contribution) / RR_THRESHOLD;
			}
			bool terminate = sample::rand01() < termination_prob;

			// recursive step: trace scattered ray in wi direction (if not terminated by RR)
			vec3 Li = vec3(0);
			if (!terminate) {
				vec3 refl_offset = wi_hemi.z > 0 ? EPSILON * n : -EPSILON * n;
				Ray ray_refl(hit_p + refl_offset, wi_world); // alright I give up fighting epsilon for now...
#if USE_DIRECT_LIGHT
				if (bsdf->is_delta) ray_refl.receive_le = true;
#endif
				// if it has some termination probability, weigh it more if it's not terminated
				Li = trace_ray(ray_refl, ray_depth + 1, debug) * (1.0f / (1.0f - termination_prob));
			}

			L += Li * f * costhetai / pdf;
		}
#endif
		if (debug) LOGF("level %d returns: (%f %f %f)", ray_depth, L.x, L.y, L.z);
    return clamp(L, vec3(0), vec3(INF));
  }
  return vec3(0);
}
