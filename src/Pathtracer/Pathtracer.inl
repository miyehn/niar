#include "Primitive.hpp"
#include "Camera.hpp"

void Pathtracer::generate_pixel_offsets() {
	pixel_offsets.clear();
	size_t sqk = std::ceil(sqrt(Cfg.Pathtracer.MinRaysPerPixel->get()));
	size_t num_offsets = pow(sqk, 2);
	TRACEF("generating %d pixel offsets", num_offsets);
	
	// canonical arrangement
	for (int j=0; j<sqk; j++) {
		for (int i=0; i<sqk; i++) {
			vec2 p;
			p.x = (i + (j + sample::rand01()) / sqk) / sqk;
			p.y = (j + (i + sample::rand01()) / sqk) / sqk;
			pixel_offsets.push_back(p);
		}
	}
	// shuffle canonical arrangement
	for (int j=0; j<sqk; j++) {
		int k = std::floor(j + sample::rand01() * (sqk - j));
		for (int i=0; i<sqk; i++) {
			float tmp = pixel_offsets[j*sqk + i].x;
			pixel_offsets[j*sqk + i].x = pixel_offsets[k*sqk + i].x;
			pixel_offsets[k*sqk + i].x = tmp;
		}
	}
	for (int i=0; i<sqk; i++) {
		int k = floor(i + sample::rand01() * (sqk - i));
		for (int j=0; j<sqk; j++) {
			float tmp = pixel_offsets[j*sqk + i].y;
			pixel_offsets[j*sqk + i].y = pixel_offsets[j*sqk + k].y;
			pixel_offsets[j*sqk + k].y = tmp;
		}
	}

}

void Pathtracer::generate_rays(std::vector<RayTask>& tasks, size_t index) {
	tasks.clear();

	size_t w = index % width;
	size_t h = index / width;
	float fov = Camera::Active->fov;
	float half_width = float(width) / 2.0f;
	float half_height = float(height) / 2.0f;
	// the raytraced image plane is at plane z = -1. Supposed k is its size in half.
	float k_y = tan(fov / 2.0f);
	float k_x = k_y * Camera::Active->aspect_ratio;

	RayTask task;
	Ray& ray = task.ray;
	bool jittered = Cfg.Pathtracer.UseJitteredSampling;
	for (int i = 0; i < (jittered ? pixel_offsets.size() : Cfg.Pathtracer.MinRaysPerPixel->get()); i++) {
		vec2 offset = jittered ? pixel_offsets[i] : sample::unit_square_uniform();

		ray.o = Camera::Active->position;
		ray.tmin = 0.0;
		ray.tmax = INF;

		// dx, dy: deviation from canvas center, normalized to range [-1, 1]
		float dx = (w + offset.x - half_width) / half_width;
		float dy = (h + offset.y - half_height) / half_height;

		vec3 d_unnormalized_c = vec3(k_x * dx, k_y * dy, -1);
		vec3 d_unnormalized_w = Camera::Active->camera_to_world_rotation() * d_unnormalized_c;
		ray.d = normalize(d_unnormalized_w);

		if (Cfg.Pathtracer.UseDOF->get()) {
			vec3 focal_p = ray.o + FocalDistance->get() * d_unnormalized_w;

			vec3 aperture_shift_cam = vec3(sample::unit_disc_uniform() * ApertureRadius->get(), 0);
			vec3 aperture_shift_world = Camera::Active->camera_to_world_rotation() * aperture_shift_cam;
			ray.o = Camera::Active->position + aperture_shift_world;
			ray.d = normalize(focal_p - ray.o);
		}

		tasks.push_back(task);
	}
}

vec3 Pathtracer::raytrace_pixel(size_t index, bool ispc) {
	std::vector<RayTask> tasks;
	generate_rays(tasks, index);

	vec3 result = vec3(0);
	for (size_t i = 0; i < tasks.size(); i++) {
		ispc ? trace_ray_ispc(tasks[i], 0) : trace_ray(tasks[i], 0, false);
		result += tasks[i].output;
	}

	result *= 1.0f / float(tasks.size());
	result = clamp(result, vec3(0), vec3(1));
	return result;
}

// DEBUG ONLY!!!
void Pathtracer::raytrace_debug(size_t index) {
	logged_rays.clear();
	logged_rays.push_back(Camera::Active->position);

	int w = index % width;
	int h = index / width;
	RayTask task;
	generate_one_ray(task, w, h);

	trace_ray(task, 0, true);
	vec3& color = task.output;
	LOGF("result color: %f %f %f", color.x, color.y, color.z);
	
	// upload ray vertices
	glBindBuffer(GL_ARRAY_BUFFER, loggedrays_vbo);
	glBufferData(GL_ARRAY_BUFFER, logged_rays.size()*sizeof(vec3), logged_rays.data(), GL_STREAM_DRAW);

	LOG("--------------------------------------");
}

void Pathtracer::generate_one_ray(RayTask& task, int x, int y) {

	Ray& ray = task.ray;

	ray.o = Camera::Active->position;
	ray.tmin = 0.0f;
	ray.tmax = INF;

	float fov = Camera::Active->fov;
	// dx, dy: deviation from canvas center, normalized to range [-1, 1]
	vec2 offset = vec2(0.5f, 0.5f);
	float half_width = float(width) / 2.0f;
	float half_height = float(height) / 2.0f;
	float dx = (x + offset.x - half_width) / half_width;
	float dy = (y + offset.y - half_height) / half_height;
	// the raytraced image plane is at plane z = -1. Supposed k is its size in half.
	float k_y = tan(fov / 2.0f);
	float k_x = k_y * Camera::Active->aspect_ratio;

	vec3 d_cam = normalize(vec3(k_x * dx, k_y * dy, -1));
	ray.d = Camera::Active->camera_to_world_rotation() * d_cam;
}

float Pathtracer::depth_of_first_hit(int x, int y) {
	RayTask task;
	generate_one_ray(task, x, y);
	
	// info of closest hit
	double t; vec3 n;
	for (size_t i = 0; i < primitives.size(); i++) {
		primitives[i]->intersect(task.ray, t, n, true);
	}

	t *= dot(task.ray.d, Camera::Active->forward());

	return float(t);
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
inline float brightness(const vec3& color) {
	return 0.2989f * color.r + 0.587f * color.g + 0.114 * color.b;
}

void Pathtracer::trace_ray(RayTask& task, int ray_depth, bool debug) {
	if (ray_depth >= Cfg.Pathtracer.MaxRayDepth) return;

	Ray& ray = task.ray;

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
		if (debug) logged_rays.push_back(hit_p);
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
		if (Cfg.Pathtracer.UseDirectLight) {
			// totally a hack...
			if (ray_depth == 0 || ray.receive_le || !bsdf->is_emissive) L += bsdf->get_emission();
		} else {
			L += bsdf->get_emission();
		}


		if (Cfg.Pathtracer.UseDirectLight) {
			//---- direct light contribution ----
			if (!bsdf->is_delta) {
				for (size_t i = 0; i < lights.size(); i++) {

					// Mesh lights
					if (lights[i]->type == PathtracerLight::AreaLight)
					{
						AreaLight* area_light = dynamic_cast<AreaLight*>(lights[i]);
						float each_sample_weight = 1.0f / Cfg.Pathtracer.AreaLightSamples;
						for (size_t j = 0; j < Cfg.Pathtracer.AreaLightSamples; j++) {

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
		}
		task.output += task.contribution * L;

#if 1 // indirect lighting (recursive)

		if ((Cfg.Pathtracer.UseDirectLight && !bsdf->is_emissive) ||
			 !(Cfg.Pathtracer.UseDirectLight)) {
			if (debug) {
				LOGF("---- hit at depth %d at (%f %f %f) ----", ray_depth, hit_p.x, hit_p.y, hit_p.z);
			}

			float pdf;
			vec3 f = bsdf->sample_f(pdf, wi_hemi, wo_hemi, debug);

			// transform wi back to world space
			wi_world = h2w * wi_hemi;
			costhetai = abs(dot(n, wi_world)); 

			if (debug) {
				LOGF("wo: %f %f %f (normalized to %f %f %f)", 
						wo_world.x, wo_world.y, wo_world.z, wo_hemi.x, wo_hemi.y, wo_hemi.z);
				LOGF("wi: %f %f %f (normalized to %f %f %f)", 
						wi_world.x, wi_world.y, wi_world.z, wi_hemi.x, wi_hemi.y, wi_hemi.z);
			}

			// russian roulette
			float termination_prob = 0.0f;
			ray.rr_contribution *= brightness(f) * costhetai;
			if (ray.rr_contribution < Cfg.Pathtracer.RussianRouletteThreshold) {
				termination_prob = (Cfg.Pathtracer.RussianRouletteThreshold - ray.rr_contribution) 
					/ Cfg.Pathtracer.RussianRouletteThreshold;
			}
			bool terminate = sample::rand01() < termination_prob;

			// recursive step: trace scattered ray in wi direction (if not terminated by RR)
			vec3 Li = vec3(0);
			if (!terminate) {
				vec3 refl_offset = wi_hemi.z > 0 ? EPSILON * n : -EPSILON * n;
				Ray ray_refl(hit_p + refl_offset, wi_world); // alright I give up fighting epsilon for now...
				task.ray = ray_refl;
				task.contribution *= f * costhetai / pdf * (1.0f / (1.0f - termination_prob));
				if (Cfg.Pathtracer.UseDirectLight && bsdf->is_delta) ray_refl.receive_le = true;
				// if it has some termination probability, weigh it more if it's not terminated
				trace_ray(task, ray_depth + 1, debug);
			} else if (debug) {
				LOG("terminated by russian roulette");
			}
		}
#endif
		if (debug) LOGF("level %d returns: (%f %f %f)", ray_depth, L.x, L.y, L.z);
	}
}

void Pathtracer::trace_ray_ispc(RayTask& task, int ray_depth) {
	if (ray_depth >= Cfg.Pathtracer.MaxRayDepth) return;

	Ray& ray = task.ray;

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

		L += bsdf->get_emission();
		task.output += task.contribution * L;

		//---- indirect lighting (recursive) ----

		float pdf;
		vec3 f = bsdf->sample_f(pdf, wi_hemi, wo_hemi, false);

		// transform wi back to world space
		wi_world = h2w * wi_hemi;
		costhetai = abs(dot(n, wi_world)); 

		// russian roulette
		float termination_prob = 0.0f;
		ray.rr_contribution *= brightness(f) * costhetai;
		if (ray.rr_contribution < Cfg.Pathtracer.RussianRouletteThreshold) {
			termination_prob = (Cfg.Pathtracer.RussianRouletteThreshold - ray.rr_contribution) 
				/ Cfg.Pathtracer.RussianRouletteThreshold;
		}
		bool terminate = sample::rand01() < termination_prob;

		// recursive step: trace scattered ray in wi direction (if not terminated by RR)
		vec3 Li = vec3(0);
		if (!terminate) {
			vec3 refl_offset = wi_hemi.z > 0 ? EPSILON * n : -EPSILON * n;
			Ray ray_refl(hit_p + refl_offset, wi_world); // alright I give up fighting epsilon for now...
			task.ray = ray_refl;
			task.contribution *= f * costhetai / pdf * (1.0f / (1.0f - termination_prob));
			// if it has some termination probability, weigh it more if it's not terminated
			trace_ray_ispc(task, ray_depth + 1);
		}
	}
}
