#include "ShaderSimulator.h"
#include "Misc.h"
#include "Log.h"

using namespace glm;

namespace myn::sky {

struct AtmosphereSample {
	vec3 rayleighScattering;
	vec3 mieScattering;
	vec3 scattering;
	vec3 absorption;
};

vec3 ws2es(vec3 pos, float bottomRadius) {
	vec3 res = pos * 0.001f;
	res.z += bottomRadius;
	return res;
}

vec3 projectToPerpendicularPlane(vec3 v, vec3 n) {
	vec3 parallelComponent = dot(v, n) * n;
	return v - parallelComponent;
}

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

vec2 UvToTransmittanceLutParams(float bottomRadius, float topRadius, vec2 uv) {
	float x_mu = uv.x;
	float x_r = uv.y;

	float viewHeight;
	float viewZenithCosAngle;

	float H = sqrt(topRadius * topRadius - bottomRadius * bottomRadius);
	float rho = H * x_r;
	viewHeight = sqrt(rho * rho + bottomRadius * bottomRadius);

	float d_min = topRadius - viewHeight;
	float d_max = rho + H;
	float d = d_min + x_mu * (d_max - d_min);

	// well ok. makes geometric sense now. made a tiny change to show I understood the code..
	viewZenithCosAngle = d == 0.0 ? 1.0f : (topRadius * topRadius - viewHeight * viewHeight - d * d) / (2.0 * viewHeight * d);
	viewZenithCosAngle = clamp(viewZenithCosAngle, -1.0f, 1.0f);

	return vec2(viewHeight, viewZenithCosAngle);
}

vec2 TransmittanceLutParamsToUv(float bottomRadius, float topRadius, float viewHeight, float viewZenithCosAngle) {
	float discriminant = viewHeight * viewHeight * (viewZenithCosAngle * viewZenithCosAngle - 1.0) + topRadius * topRadius;
	float d = max(0.0f, (-viewHeight * viewZenithCosAngle + sqrt(discriminant))); // Distance to atmosphere boundary

	float H = sqrt(max(0.0f, topRadius * topRadius - bottomRadius * bottomRadius));
	float rho = sqrt(max(0.0f, viewHeight * viewHeight - bottomRadius * bottomRadius));

	float d_min = topRadius - viewHeight;
	float d_max = rho + H;
	float x_mu = (d - d_min) / (d_max - d_min);
	float x_r = rho / H;

	return vec2(x_mu, x_r);
}

vec2 SkyViewLutParamsToUv(vec3 cameraPosES, vec3 dir2sun, vec3 viewDir) {
	vec3 dir2zenith = normalize(cameraPosES);

	vec3 dir2sunHorizontal = projectToPerpendicularPlane(dir2sun, dir2zenith);
	if (dir2sunHorizontal.x == 0 && dir2sunHorizontal.y == 0 && dir2sunHorizontal.z == 0) {
		// sun is straight above head (hack)
		dir2sunHorizontal.x = 1;
	} else {
		dir2sunHorizontal = normalize(dir2sunHorizontal);
	}

	vec3 viewDirHorizontal = projectToPerpendicularPlane(viewDir, dir2zenith);
	if (viewDirHorizontal.x == 0 && viewDirHorizontal.y == 0 && viewDirHorizontal.z == 0) {
		// looking straight above head (hack)
		viewDirHorizontal.x = 1;
	} else {
		viewDirHorizontal = normalize(viewDirHorizontal);
	}

	vec2 uv;
	uv.x = -dot(viewDirHorizontal, dir2sunHorizontal) * 0.5f + 0.5f;
	uv.y = -dot(viewDir, dir2zenith) * 0.5f + 0.5f;
	return uv;
}

void UvToSkyViewLutParams(
	vec2 uv, vec3 cameraPosES, vec3 dir2sun,
	vec3& cameraPosESProxy, vec3& viewDirProxy, vec3& dir2sunProxy){

	float viewSunCosine = 1.0f - uv.x * 2.0f; // horizontal, [1, -1]
	float viewZenithCosine = 1.0f - uv.y * 2.0f; // vertical, [1, -1]
	float sunZenithCosine = dot(dir2sun, normalize(cameraPosES));

	// use these proxy params (instead of the true ones) to compute the lut
	dir2sunProxy = normalize(vec3(sqrt(1.0f - sunZenithCosine * sunZenithCosine), 0, sunZenithCosine));
	cameraPosESProxy = vec3(0.0f, 0.0f, length(cameraPosES));

	float viewZenithSine = sqrt(1.0f - viewZenithCosine * viewZenithCosine);
	float viewSunSine = sqrt(1.0f - viewSunCosine * viewSunCosine);
	viewDirProxy = normalize(vec3(
		viewZenithSine * viewSunCosine,
		viewZenithSine * viewSunSine,
		viewZenithCosine));
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

AtmosphereSample sampleAtmosphere(AtmosphereProfile atmosphere, float heightFromGroundKM) {
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
vec3 computeTransmittanceToSun(AtmosphereProfile atmosphere, float viewHeight, float viewZenithCosine) {

	float discriminant = viewHeight * viewHeight * (viewZenithCosine * viewZenithCosine - 1.0) + atmosphere.topRadius * atmosphere.topRadius;
	float distToAtmosphereTop = max(0.0f, (-viewHeight * viewZenithCosine + sqrt(discriminant))); // Distance to atmosphere boundary

	vec3 traceStartPosES = vec3(0, 0, viewHeight);
	vec3 traceDir = vec3(sqrt(1.0f - viewZenithCosine * viewZenithCosine), 0, viewZenithCosine);

	vec3 cumOpticalDepth = vec3(0);

	float numSamplesF = 40;
	const float sampleSegmentT = 0.3f;
	float t = 0;

	for (float i = 0; i < numSamplesF; i += 1.0f) {
		float newT = distToAtmosphereTop * ((i + sampleSegmentT) / numSamplesF);
		float dt = newT - t;
		t = newT;
		vec3 samplePosES = traceStartPosES + t * traceDir;
		AtmosphereSample sample = sampleAtmosphere(atmosphere, length(samplePosES) - atmosphere.bottomRadius);
		vec3 segmentOpticalDepth = dt * (sample.scattering + sample.absorption);
		cumOpticalDepth += segmentOpticalDepth;
	}

	return exp(-cumOpticalDepth);
}

vec3 sampleTransmittanceToSun(const CpuTexture *transmittanceLut, float bottomRadius, float topRadius, float viewHeight, float viewZenithCosine) {
	vec2 uv = TransmittanceLutParamsToUv(bottomRadius, topRadius, viewHeight, viewZenithCosine);
	vec4 texel = transmittanceLut->sampleBilinear(uv, CpuTexture::WM_Clamp);
	return vec3(texel.r, texel.g, texel.b);
}

vec2 computeRaymarchAtmosphereMinMaxT(vec3 startPosES, vec3 raymarchDir, vec3 earthCenterES, float bottomRadius, float topRadius) {
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

vec3 computeSkyAtmosphere(
	AtmosphereProfile atmosphere,
	const CpuTexture* transmittanceLut,
	vec3 cameraPosES,
	vec3 viewDir,
	vec3 dir2sun,
	vec3 sunLuminance,
	vec2 numSamplesMinMax)
{
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

#if 0 // fixed samples count
		float numSamplesF = 64.0f;
#else
		float upness = 1.0f - clamp(dot(normalize(cameraPosES), viewDir), 0.0f, 1.0f);
		float numSamplesF = floor(mix(numSamplesMinMax.x, numSamplesMinMax.y, upness));
#endif
		const float sampleSegmentT = 0.3f;
		float t = 0;

		for (float i = 0; i < numSamplesF; i += 1.0f) {
			// loop variables
			float newT01 = (i + sampleSegmentT) / numSamplesF;
			float newT = tMax * newT01;
			float dt = newT - t;
			t = newT;
			vec3 samplePosES = cameraPosES + t * viewDir;

			// atmosphere sample
			AtmosphereSample atmosphereSample = sampleAtmosphere(atmosphere, length(samplePosES) - atmosphere.bottomRadius);

			float viewHeight = length(samplePosES);
#if 1
			vec3 transmittanceToSun = sampleTransmittanceToSun(
				transmittanceLut,
				atmosphere.bottomRadius,
				atmosphere.topRadius,
				viewHeight,
				dot(samplePosES, dir2sun) / viewHeight);
#else
			vec3 transmittanceToSun = computeTransmittanceToSun(atmosphere, viewHeight, dot(samplePosES, dir2sun) / viewHeight);
#endif

			vec3 phaseTimesScattering = atmosphereSample.mieScattering * miePhase + atmosphereSample.rayleighScattering * rayleighPhase;

			// earth shadow
			float tEarth = raySphereIntersectNearest(samplePosES, dir2sun, earthCenterES, atmosphere.bottomRadius);
			float sunVisibility = tEarth >= 0 ? 0 : 1;

			// todo: add multiscattered light contribution here
			vec3 sunContrib = sunLuminance * sunVisibility * transmittanceToSun * phaseTimesScattering;

			vec3 segmentOpticalDepth = (atmosphereSample.scattering + atmosphereSample.absorption) * dt;
			vec3 segmentTransmittance = exp(-segmentOpticalDepth);

			// analytical solution to the integral along view ray segment (see sample code)
			L += sunContribThroughput * (sunContrib - sunContrib * segmentTransmittance) /
				 (atmosphereSample.scattering + atmosphereSample.absorption);

			sunContribThroughput *= segmentTransmittance;

		} // end for (raymarch along view ray)

		return L;

	} // end if (shouldRaymarch)

	return vec3(0);
}

void TransmittanceLutSim::runSim() {
	auto texdim = uvec2(output->getWidth(), output->getHeight());
	dispatchShader([&](uint32_t x, uint32_t y) {
		vec2 uv = vec2(float(x + 0.5f) / texdim.x, float(y + 0.5f) / texdim.y);
		vec2 transmittanceLutParams = UvToTransmittanceLutParams(atmosphere->bottomRadius, atmosphere->topRadius, uv);
		vec3 transmittanceToSun = computeTransmittanceToSun(*atmosphere, transmittanceLutParams.x, transmittanceLutParams.y);
		return vec4(transmittanceToSun, 1);
	});
}

void SkyAtmosphereSim::runSim() {
	auto texdim = uvec2(output->getWidth(), output->getHeight());

	dispatchShader([&](uint32_t x, uint32_t y) {
		vec2 uv = vec2(float(x + 0.5f) / texdim.x, float(y + 0.5f) / texdim.y);
		vec3 viewDir = uvToViewDir_longlat(uv);
		vec3 cameraPosES = ws2es(renderingParams->cameraPosWS, renderingParams->atmosphere.bottomRadius);

		vec2 skyViewUv = SkyViewLutParamsToUv(cameraPosES, renderingParams->dir2sun, viewDir);
		vec3 result = skyViewLut->sampleBilinear(skyViewUv, CpuTexture::WM_Clamp);

		return vec4(result, 1.0f);
	});
}

void sky::SkyAtmospherePostProcess::runSim() {
	ASSERT(skyTextureRaw != nullptr && output != nullptr)
	ASSERT(skyTextureRaw != output)
	auto texdim = uvec2(output->getWidth(), output->getHeight());
	dispatchShader([&](uint32_t x, uint32_t y) {
		vec2 uv = vec2(float(x + 0.5f) / texdim.x, float(y + 0.5f) / texdim.y);
		auto raw = skyTextureRaw->sampleBilinear(uv, CpuTexture::WM_Clamp);
		vec3 white_point = vec3(1.08241, 0.96756, 0.95003);
		vec3 base = 1.0f - exp(-vec3(raw.x, raw.y, raw.z) / white_point * renderingParams->exposure);
		vec3 exponent = vec3(1.0f / 2.2f);
		vec3 res = pow(base, exponent);
		return vec4(res, 1.0f);
	});
}

void sky::SkyViewLutSim::runSim() {
	auto texdim = uvec2(output->getWidth(), output->getHeight());
	dispatchShader([&](uint32_t x, uint32_t y) {
		vec2 uv = vec2(float(x + 0.5f) / texdim.x, float(y + 0.5f) / texdim.y);
		vec3 cameraPosES = ws2es(renderingParams->cameraPosWS, renderingParams->atmosphere.bottomRadius);

		vec3 cameraPosESProxy, viewDirProxy, dir2sunProxy;
		UvToSkyViewLutParams(uv, cameraPosES, renderingParams->dir2sun, cameraPosESProxy, viewDirProxy, dir2sunProxy);

		vec3 L = computeSkyAtmosphere(
			renderingParams->atmosphere, transmittanceLut,
			cameraPosESProxy,
			viewDirProxy,
			dir2sunProxy,
			renderingParams->sunLuminance,
			renderingParams->skyViewNumSamplesMinMax);

		return vec4(L, 1);
	});
}

} // namespace myn::sky
