#include "utils.glsl"

struct AtmosphereProfile {

    vec3 rayleighScattering;
    float bottomRadius;

    vec3 mieScattering;
    float topRadius;

    vec3 mieAbsorption;
    float miePhaseG;

    vec3 ozoneAbsorption;
    float ozoneMeanHeight;

    vec3 groundAlbedo; // not used for now
    float ozoneLayerWidth;
};

layout(set = 3, binding = 0) uniform SkyAtmosphereParamsBufferObject {

    AtmosphereProfile atmosphere;

    vec3 cameraPosES;
    float exposure;

    vec3 dir2sun;
    float sunAngularRadius;

    vec2 skyViewNumSamplesMinMax;

    uvec2 transmittanceLutTextureDimensions;
    uvec2 skyViewLutTextureDimensions;

} params;

//////////////////////////////////////////////////////////////////////////

struct AtmosphereSample {
    vec3 rayleighScattering;
    vec3 mieScattering;
    vec3 scattering;
    vec3 absorption;
};

vec3 projectToPerpendicularPlane(vec3 v, vec3 n) {
    vec3 parallelComponent = dot(v, n) * n;
    return v - parallelComponent;
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

float toSubUv(float u, float resolution) {
    return u - u / resolution + 0.5f / resolution;
}

float fromSubUv(float x, float resolution) {
    return (x * resolution - 0.5f) / (resolution - 1.0f);
}

#define SKYVIEWLUT_LINEAR_MAPPING 0

vec2 SkyViewLutParamsToUv(vec3 cameraPosES, vec3 dir2sun, vec3 viewDir, float bottomRadius, bool intersectGround) {
    vec2 uv;
    vec3 dir2zenith = normalize(cameraPosES);

#if SKYVIEWLUT_LINEAR_MAPPING

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

    // non-linear mapping can transform uv here depending on the sign of dot(viewDir, dir2zenith),
    // but when decoding uv there would be no way to know where the division happens...
    // in other words there's no single transform function that applies, because the horizon could appear anywhere
    uv.x = -dot(viewDirHorizontal, dir2sunHorizontal) * 0.5f + 0.5f;
    uv.y = -dot(viewDir, dir2zenith) * 0.5f + 0.5f;
#else
    float viewHeight = length(cameraPosES);
    float viewZenithCosine = dot(dir2zenith, viewDir);

    float Vhorizon = sqrt(viewHeight * viewHeight - bottomRadius * bottomRadius);
    float CosBeta = Vhorizon / viewHeight;				// GroundToHorizonCos
    float Beta = acos(CosBeta);
    float ZenithHorizonAngle = PI - Beta;

    if (!intersectGround) {
        float coord = acos(viewZenithCosine) / ZenithHorizonAngle;
        coord = 1.0f - coord;
        coord = sqrt(coord);
        coord = 1.0f - coord;
        uv.y = coord * 0.5f;
    } else {
        float coord = (acos(viewZenithCosine) - ZenithHorizonAngle) / Beta;
        coord = sqrt(coord);
        uv.y = coord * 0.5f + 0.5f;
    }

    {
        float coord = -(dot(dir2sun, viewDir)) * 0.5f + 0.5f;
        coord = sqrt(coord);
        uv.x = coord;
    }

    // Constrain uvs to valid sub texel range (avoid zenith derivative issue making LUT usage visible)
    //uv = float2(fromUnitToSubUvs(uv.x, 192.0f), fromUnitToSubUvs(uv.y, 108.0f));
#endif
    return uv;
}

void UvToSkyViewLutParams(
vec2 uv, float bottomRadius, vec3 cameraPosES, vec3 dir2sun, out vec3 cameraPosESProxy, out vec3 viewDirProxy, out vec3 dir2sunProxy) {

#if SKYVIEWLUT_LINEAR_MAPPING // straight-forward linear mapping
    float viewSunCosine = 1.0f - uv.x * 2.0f; // horizontal, [1, -1]
    float viewZenithCosine = 1.0f - uv.y * 2.0f; // vertical, [1, -1]

#else // non-linear mapping from the sample, which keeps horizon in the middle
    // Constrain uvs to valid sub texel range (avoid zenith derivative issue making LUT usage visible)
    //uv = float2(fromSubUvsToUnit(uv.x, 192.0f), fromSubUvsToUnit(uv.y, 108.0f));

    float viewHeight = length(cameraPosES);

    float Vhorizon = sqrt(viewHeight * viewHeight - bottomRadius * bottomRadius);
    float CosBeta = Vhorizon / viewHeight;				// GroundToHorizonCos
    float Beta = acos(CosBeta);
    float ZenithHorizonAngle = PI - Beta;

    float viewZenithCosine;
    if (uv.y < 0.5f) {
        float coord = 2.0*uv.y;
        coord = 1.0 - coord;
        coord *= coord;
        coord = 1.0 - coord;
        viewZenithCosine = cos(ZenithHorizonAngle * coord);
    } else {
        float coord = uv.y * 2.0f - 1.0f;
        coord *= coord;
        viewZenithCosine = cos(ZenithHorizonAngle + Beta * coord);
    }

    float coord = uv.x;
    coord *= coord;
    float viewSunCosine = -(coord * 2.0f - 1.0f);
#endif
    float sunZenithCosine = dot(dir2sun, normalize(cameraPosES));

    float viewZenithSine = sqrt(1.0f - viewZenithCosine * viewZenithCosine);
    float viewSunSine = sqrt(1.0f - viewSunCosine * viewSunCosine);

    dir2sunProxy = normalize(vec3(sqrt(1.0f - sunZenithCosine * sunZenithCosine), 0, sunZenithCosine));
    cameraPosESProxy = vec3(0.0f, 0.0f, length(cameraPosES));
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

    AtmosphereSample s;
    s.rayleighScattering = rayleighDensity * atmosphere.rayleighScattering;
    s.mieScattering = mieDensity * atmosphere.mieScattering;
    s.scattering = s.rayleighScattering + s.mieScattering; // ozone does not scatter
    s.absorption =
        // rayleigh does not absorb
        mieDensity * atmosphere.mieAbsorption +
        ozoneDensity * atmosphere.ozoneAbsorption;

    return s;
}
// assume sample pos is within the atmosphere already
vec3 computeTransmittanceToSun(AtmosphereProfile atmosphere, float viewHeight, float viewZenithCosine) {

    float discriminant = viewHeight * viewHeight * (viewZenithCosine * viewZenithCosine - 1.0) + atmosphere.topRadius * atmosphere.topRadius;
    float distToAtmosphereTop = max(0.0f, (-viewHeight * viewZenithCosine + sqrt(discriminant))); // Distance to atmosphere boundary

    vec3 traceStartPosES = vec3(0, 0, viewHeight);
    vec3 traceDir = vec3(sqrt(1.0f - viewZenithCosine * viewZenithCosine), 0, viewZenithCosine);

    vec3 cumOpticalDepth = vec3(0, 0, 0);

    float numSamplesF = 40;
    const float sampleSegmentT = 0.3f;
    float t = 0;

    for (float i = 0; i < numSamplesF; i += 1.0f) {
        float newT = distToAtmosphereTop * ((i + sampleSegmentT) / numSamplesF);
        float dt = newT - t;
        t = newT;
        vec3 samplePosES = traceStartPosES + t * traceDir;
        AtmosphereSample s = sampleAtmosphere(atmosphere, length(samplePosES) - atmosphere.bottomRadius);
        vec3 segmentOpticalDepth = dt * (s.scattering + s.absorption);
        cumOpticalDepth += segmentOpticalDepth;
    }

    return exp(-cumOpticalDepth);
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
