#include "ShaderSimulator.h"
#include "Misc.h"
#include "Log.h"

using namespace glm;

namespace myn::sky {

struct AtmosphereParams {
	float bottomRadius;
	float topRadius;

	vec3 rayleighScattering;
	// rayleigh absorption = 0

	vec3 mieScattering;
	vec3 mieAbsorption;

	float miePhaseG;

	// ozone scattering = 0
	vec3 ozoneAbsorption;

	// ozone distribution
	float ozoneMeanHeight;
	float ozoneLayerWidth;

	vec3 groundAlbedo;
};

struct AtmosphereSample {
	vec3 rayleighScattering;
	vec3 mieScattering;
	vec3 scattering;
	vec3 absorption;
};

vec3 uvToViewDir_longlat(vec2 uv) {
	float theta = (uv.x - 0.5f) * TWO_PI;
	float phi = (uv.y - 0.5f) * PI;
	float x = cos(-theta);
	float y = sin(-theta);
	float z = sin(-phi);
	return normalize(vec3(x, y, z));
}

vec2 viewDirToUv_longlat(vec3 dir) {
	float phi = atan(dir.y, dir.x);
	float theta = asin(dir.z);
	vec2 uv = vec2(-phi * ONE_OVER_TWO_PI + 0.5f, -theta * ONE_OVER_PI + 0.5f);
	return uv;
}

// took this straight from sample code because I'm too lazy to write my own
// - r0: ray origin
// - rd: normalized ray direction
// - s0: sphere center
// - sR: sphere radius
// - Returns distance from r0 to first intersecion with sphere,
//   or -1.0 if no intersection.
float raySphereIntersectNearest(vec3 r0, vec3 rd, vec3 s0, float sR) {
	float a = dot(rd, rd);
	vec3 s0_r0 = r0 - s0;
	float b = 2.0f * dot(rd, s0_r0);
	float c = dot(s0_r0, s0_r0) - (sR * sR);
	float delta = b * b - 4.0 * a * c;
	if (delta < 0.0 || a == 0.0) {
		return -1.0;
	}
	float sol0 = (-b - sqrt(delta)) / (2.0f * a);
	float sol1 = (-b + sqrt(delta)) / (2.0f * a);
	if (sol0 < 0.0 && sol1 < 0.0) {
		return -1.0;
	}
	if (sol0 < 0.0) {
		return max(0.0f, sol1);
	} else if (sol1 < 0.0) {
		return max(0.0f, sol0);
	}
	return max(0.0f, min(sol0, sol1));
}

vec3 ws2es(vec3 pos, float bottomRadius) {
	vec3 res = pos * 0.001f;
	res.z += bottomRadius;
	return res;
}

vec3 es2ws(vec3 pos, float bottomRadius) {
	vec3 res = pos;
	res.z -= bottomRadius;
	res *= 1000.0f;
	return res;
}

float computeRayleighPhase(float cosTheta) {
	const float denom = 16.0f * PI;
	return 3 * (1 + cosTheta * cosTheta) / denom;
}

float computeHgPhase(float g, float cosTheta) {
#if 0
	// https://omlc.org/classroom/ece532/class3/hg.html
	// seems wrong though (not normalized)
	float gSq = g * g;
	float denom = pow(1 + gSq - 2*g * cosTheta, 1.5f);
	return 0.5f * (1 - gSq) / denom;
#else
	// took straight from the sample
	float k = 3.0 / (8.0 * PI) * (1.0 - g * g) / (2.0 + g * g);
	return k * (1.0 + cosTheta * cosTheta) / pow(1.0 + g * g - 2.0 * g * -cosTheta, 1.5);
#endif
}

AtmosphereSample sampleAtmosphere(AtmosphereParams atmosphere, float heightFromGroundKM) {
	heightFromGroundKM = max(0.0f, heightFromGroundKM); // if underground, clamp to ground.

	float rayleighDensity = exp(-heightFromGroundKM * 0.125f);
	float mieDensity = exp(-heightFromGroundKM * 0.833f);
	float distToMeanOzoneHeight = abs(heightFromGroundKM - atmosphere.ozoneMeanHeight);
	float halfOzoneLayerWidth = atmosphere.ozoneLayerWidth * 0.5f;
	float ozoneDensity = max(0.0f, halfOzoneLayerWidth - distToMeanOzoneHeight) / halfOzoneLayerWidth;

	AtmosphereSample sample;
	sample.rayleighScattering = rayleighDensity * atmosphere.rayleighScattering;
	sample.mieScattering = mieDensity * atmosphere.mieScattering;
	sample.scattering = sample.rayleighScattering + sample.mieScattering; // ozone does not scatter
	sample.absorption =
		// rayleigh does not absorb
		mieDensity * atmosphere.mieAbsorption +
		ozoneDensity * atmosphere.ozoneAbsorption;

	return sample;
}

// assume sample pos is within the atmosphere already
// todo: replace this with an lut lookup
vec3 computeTransmittanceToSun(AtmosphereParams atmosphere, vec3 startPosES, vec3 dir2sun) {

	// todo: do the regular call to helper to figure out tmin and tmax?
	float tMax = raySphereIntersectNearest(startPosES, dir2sun, vec3(0), atmosphere.topRadius);

	vec3 cumOpticalDepth = vec3(0);

	float numSamplesF = 40;
	const float sampleSegmentT = 0.3f;
	float t = 0;

	for (float i = 0; i < numSamplesF; i += 1.0f) {
		float newT = tMax * ((i + sampleSegmentT) / numSamplesF);
		float dt = newT - t;
		t = newT;
		vec3 samplePosES = startPosES + t * dir2sun;
		AtmosphereSample sample = sampleAtmosphere(atmosphere, length(samplePosES) - atmosphere.bottomRadius);
		vec3 segmentOpticalDepth = dt * (sample.scattering + sample.absorption);
		cumOpticalDepth += segmentOpticalDepth;
	}

	return exp(-cumOpticalDepth);
}

vec2 computeRaymarchAtmosphereMinMaxT(vec3 startPosES, vec3 raymarchDir, vec3 earthCenterES, float bottomRadius,
									  float topRadius) {
	// get the range to raymarch through
	const float tBottom = raySphereIntersectNearest(startPosES, raymarchDir, earthCenterES, bottomRadius);
	const float tTop = raySphereIntersectNearest(startPosES, raymarchDir, earthCenterES, topRadius);
	const bool isectWithGround = tBottom >= 0;
	const bool isectWithTop = tTop >= 0;
	float tMin, tMax;
	if (isectWithGround) {
		if (isectWithTop) {
			float viewHeight = length(startPosES);
			if (viewHeight < bottomRadius) {
				// underground
				tMin = 0;
				tMax = tTop;
			} else if (viewHeight < topRadius) {
				// within atmosphere, looking down
				tMin = 0;
				tMax = tBottom;
			} else {
				// from outer space, looking at planet
				tMin = tTop;
				tMax = tBottom;
			}
		} else {
			// shouldn't get here (no isect with top but isect with ground is impossible)
			DEBUG_BREAK;
		}
	} else { // not intersecting w ground
		if (isectWithTop) {
			float viewHeight = length(startPosES);
			if (viewHeight < topRadius) {
				// within atmosphere, looking up into the sky
				tMin = 0;
				tMax = tTop;
			} else {
				// from outer space, view ray penetrate through atmosphere and to outer space again
				tMin = tTop;
				tMax = raySphereIntersectNearest(
					startPosES + raymarchDir * topRadius * 2.01f,
					-raymarchDir,
					earthCenterES,
					topRadius);
			}
		} else {
			// in outer space, not looking at planet
			tMin = -1;
			tMax = -1;
		}
	}
	return vec2(tMin, tMax);
}

void SkyAtmosphereSim::runSim() {
	auto texdim = uvec2(outputTexture.getWidth(), outputTexture.getHeight());

	// parameters
	// the paper labeled extinction as absorption which is wrong, fuck
	float mieScattering = 0.003996f;
	float mieExtinction = 0.00440f;
	float mieAbsorption = mieExtinction - mieScattering;
	AtmosphereParams atmosphere = {
		.bottomRadius = 6360,
		.topRadius = 6460,
		.rayleighScattering = {
			5.802f * 1e-3,
			13.558f * 1e-3,
			33.1f * 1e-3
		},
		// rayleigh absorption = 0
		.mieScattering = {
			mieScattering, mieScattering, mieScattering
		},
		.mieAbsorption = {
			mieAbsorption, mieAbsorption, mieAbsorption
		},
		.miePhaseG = 0.8f,
		// ozone scattering = 0
		.ozoneAbsorption = {
			0.650f * 1e-3,
			1.881f * 1e-3,
			0.085f * 1e-3
		},
		.ozoneMeanHeight = 25,
		.ozoneLayerWidth = 30,
		.groundAlbedo = {0.3f, 0.3f, 0.3f}
	};

	// earth space: origin at center of planet
	vec3 cameraPosWS = {0, 0, 500};
	vec3 dir2sun = normalize(vec3(1, 0, -0.01f));
	vec3 sunL = vec3(1);

	dispatchShader([&](uint32_t x, uint32_t y) {
		vec2 uv = vec2(float(x + 0.5f) / texdim.x, float(y + 0.5f) / texdim.y);
		vec3 viewDir = uvToViewDir_longlat(uv);

		vec3 cameraPosES = ws2es(cameraPosWS, atmosphere.bottomRadius);
		vec3 earthCenterES = vec3(0);

		vec2 tMinMax = computeRaymarchAtmosphereMinMaxT(cameraPosES, viewDir, earthCenterES, atmosphere.bottomRadius,
														atmosphere.topRadius);
		bool shouldRaymarch = tMinMax.x >= 0 && tMinMax.y >= 0;

		if (shouldRaymarch) {

			// prepare to actually raymarch: let [tmin, tmax] be [0, length of entire raymarch segment]
			vec3 raymarchStartPosES = cameraPosES + tMinMax.x * viewDir;
			float tMax = tMinMax.y - tMinMax.x;

			// phase functions
			float cosTheta_viewDir_dir2sun = dot(viewDir, dir2sun);
			float rayleighPhase = computeRayleighPhase(cosTheta_viewDir_dir2sun);
			float miePhase = computeHgPhase(atmosphere.miePhaseG, -cosTheta_viewDir_dir2sun);

			// todo: wtf is this
			//tMax = min(tMax, 200.0f);

			vec3 L = vec3(0);
			vec3 sunContribThroughput = vec3(1);

			float numSamplesF = 64.0f; // todo: variable sample count
			const float sampleSegmentT = 0.3f;
			float t = 0;

			int numSamplesI = floor(numSamplesF);
			for (int i = 0; i < numSamplesI; i++) {
				// loop variables
				float newT = tMax * ((float(i) + sampleSegmentT) / numSamplesF);
				float dt = newT - t;
				t = newT;
				vec3 samplePosES = cameraPosES + t * viewDir;

				AtmosphereSample atmosphereSample = sampleAtmosphere(atmosphere,
																	 length(samplePosES) - atmosphere.bottomRadius);
				vec3 transmittanceToSun = computeTransmittanceToSun(atmosphere, samplePosES, dir2sun);
				vec3 phaseTimesScattering =
					atmosphereSample.mieScattering * miePhase + atmosphereSample.rayleighScattering * rayleighPhase;

				// earth shadow
				float tEarth = raySphereIntersectNearest(samplePosES, dir2sun, earthCenterES, atmosphere.bottomRadius);
				float sunVisibility = tEarth >= 0 ? 0 : 1;

				// todo: add multiscattered light contribution here
				vec3 sunContrib = sunL * sunVisibility * transmittanceToSun * phaseTimesScattering;

				vec3 segmentOpticalDepth = (atmosphereSample.scattering + atmosphereSample.absorption) * dt;
				vec3 segmentTransmittance = exp(-segmentOpticalDepth);

				// analytical solution to the integral along view ray segment (see sample code)
				L += sunContribThroughput * (sunContrib - sunContrib * segmentTransmittance) /
					 (atmosphereSample.scattering + atmosphereSample.absorption);

				sunContribThroughput *= segmentTransmittance;

			} // end for (raymarch along view ray)

			vec3 white_point = vec3(1.08241, 0.96756, 0.95003);
			float exposure = 10.0;
			vec3 base = 1.0f - exp(-L / white_point * exposure);
			vec3 exponent = vec3(1.0f / 2.2f);
			vec3 res = pow(base, exponent);
			return vec4(res, 1.0f);

		} // end if (shouldRaymarch)

		return vec4(0, 0, 0, 1);

	});
}
}

