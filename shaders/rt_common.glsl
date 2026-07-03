#ifndef _SHADER_INCLUDE_RT_COMMON
#define _SHADER_INCLUDE_RT_COMMON

#include "bindless_resources.glsl"

struct RayHitResult
{
    bool committed;
    uint intersectionType;
    uint instanceCustomIndex;
    uint primitiveIndex;
    vec2 barycentrics;
    float hitT;
    mat4x3 worldToObject;
};

struct HitSurface
{
    vec3 worldPosition;
    vec3 objectPosition;
    vec3 worldNormal;
    vec2 uv;
    vec3 outgoingDirection;
    GpuMaterial material;
};

layout(buffer_reference, std430, buffer_reference_align = 4) readonly buffer FloatBuffer {
    float values[];
};

layout(buffer_reference, std430, buffer_reference_align = 4) readonly buffer UintBuffer {
    uint values[];
};

RayHitResult makeMissHitResult()
{
    return RayHitResult(false, gl_RayQueryCommittedIntersectionNoneEXT, ~0u, ~0u, vec2(-1.0), -1.0, mat4x3(0.0));
}

RayHitResult interpretRayQuery(rayQueryEXT rq)
{
    // the second boolean arg being true: want the final committed intersection
    // hit opaque triangle (forced by gl_RayFlagsOpaqueEXT, or VK_GEOMETRY_OPAQUE_BIT_KHR when building BLAS) -> internally confirmed
    // hit non-opaque triangle -> shader must confirm the hit
    // hit AABB (need BLAS to be built as procedural geometry not triangle geometry) -> shader must generate an intersection
    uint intersectionType = rayQueryGetIntersectionTypeEXT(rq, true);

    RayHitResult result = makeMissHitResult();
    if (intersectionType == gl_RayQueryCommittedIntersectionNoneEXT) {
        // it's a miss
        return result;
    }

    // it's a hit
    result.committed = true;

    // intersection types can be: None (miss), Triangle (triangle geometry), or Generated (AABB geometry), which is used for shader-defined procedural geometry
    result.intersectionType = intersectionType;

    result.instanceCustomIndex = rayQueryGetIntersectionInstanceCustomIndexEXT(rq, true);
    result.primitiveIndex = rayQueryGetIntersectionPrimitiveIndexEXT(rq, true);
    result.hitT = rayQueryGetIntersectionTEXT(rq, true);
    result.worldToObject = rayQueryGetIntersectionWorldToObjectEXT(rq, true);
    if (intersectionType == gl_RayQueryCommittedIntersectionTriangleEXT) {
        result.barycentrics = rayQueryGetIntersectionBarycentricsEXT(rq, true);
    }
    return result;
}

RayHitResult traceRay(accelerationStructureEXT tlas, vec3 origin, vec3 direction, float tmin, float tmax)
{
    rayQueryEXT rq;
    rayQueryInitializeEXT(
        rq,
        tlas,
    // ray flags:
        0
        // | gl_RayFlagsTerminateOnFirstHitEXT // with this flag present, may terminate at ANY hit, not necessarily closest
        // | gl_RayFlagsSkipClosestHitShaderEXT // only relevant when in ray tracing pipeline
        | gl_RayFlagsOpaqueEXT,
        0xFF, // cull mask, see: https://github.com/KhronosGroup/GLSL/blob/d2470a0a124bbb8c90a3576aca94694bd2f789e0/extensions/ext/GLSL_EXT_ray_query.txt#L286
    // basically, the 8 bits will be combined with the mask field in VkAccelerationStructureInstanceKHR. Visible if result is non-zero.
        origin,
        tmin,
        direction,
        tmax);
    rayQueryProceedEXT(rq);

    return interpretRayQuery(rq);
}

// read vertexIndex from index buffer. the result will be used to read into vertex buffer
bool readGeometryIndex(GpuGeometryRecord geometry, uint elementIndex, out uint index)
{
    index = ~0u;
    if (elementIndex >= geometry.indexCount) {
        return false;
    }

    UintBuffer indexBuffer = UintBuffer(geometry.indexBufferAddress);
    if (geometry.indexType == GPU_GEOMETRY_INDEX_TYPE_UINT16) {
        uint byteOffset = geometry.indexBufferOffsetBytes + elementIndex * 2u;
        uint packedIndices = indexBuffer.values[byteOffset >> 2u]; // if each index is 16 bits, packedIndices contains 2
        index = (byteOffset & 2u) == 0u
        ? packedIndices & 0xffffu
        : packedIndices >> 16u;
        return true;
    }

    if (geometry.indexType == GPU_GEOMETRY_INDEX_TYPE_UINT32) {
        uint byteOffset = geometry.indexBufferOffsetBytes + elementIndex * 4u;
        if ((byteOffset & 3u) != 0u) { // basically says byte offset should be multiply of 4
                                       return false;
        }
        index = indexBuffer.values[byteOffset >> 2u];
        return true;
    }

    return false;
}

bool readTriangleVertexIndices(RayHitResult hitResult, GpuGeometryRecord geometry, out uvec3 indices)
{
    indices = uvec3(~0u);
    if (hitResult.intersectionType != gl_RayQueryCommittedIntersectionTriangleEXT) {
        return false;
    }
    if (geometry.indexCount < 3u || hitResult.primitiveIndex >= geometry.indexCount / 3u) {
        return false;
    }

    uint firstIndex = hitResult.primitiveIndex * 3u;
    return
    readGeometryIndex(geometry, firstIndex, indices.x) &&
    readGeometryIndex(geometry, firstIndex + 1u, indices.y) &&
    readGeometryIndex(geometry, firstIndex + 2u, indices.z);
}

bool readVertexVec3(GpuGeometryRecord geometry, uint vertexIndex, uint attribOffset, out vec3 value)
{
    value = vec3(0.0);
    if (vertexIndex >= geometry.vertexCount) {
        return false;
    }

    uint byteOffset =
    geometry.vertexBufferOffsetBytes +
    vertexIndex * geometry.vertexStride +
    attribOffset;
    if ((byteOffset & 3u) != 0u) {
        return false;
    }

    FloatBuffer vertexBuffer = FloatBuffer(geometry.vertexBufferAddress);
    uint floatOffset = byteOffset >> 2u;
    value = vec3(
    vertexBuffer.values[floatOffset],
    vertexBuffer.values[floatOffset + 1u],
    vertexBuffer.values[floatOffset + 2u]);
    return true;
}

bool readVertexUv(GpuGeometryRecord geometry, uint vertexIndex, out vec2 uv)
{
    uv = vec2(0.0);
    if (vertexIndex >= geometry.vertexCount) {
        return false;
    }

    uint byteOffset =
    geometry.vertexBufferOffsetBytes +
    vertexIndex * geometry.vertexStride +
    geometry.uvAttribOffset;
    if ((byteOffset & 3u) != 0u) {
        return false;
    }

    FloatBuffer vertexBuffer = FloatBuffer(geometry.vertexBufferAddress);
    uint floatOffset = byteOffset >> 2u;
    uv = vec2(vertexBuffer.values[floatOffset], vertexBuffer.values[floatOffset + 1u]);
    return true;
}

bool reconstructHitUv(RayHitResult hitResult, GpuGeometryRecord geometry, out vec2 uv)
{
    uv = vec2(0.0);
    uvec3 indices;
    if (!readTriangleVertexIndices(hitResult, geometry, indices)) {
        return false;
    }

    vec2 uv0;
    vec2 uv1;
    vec2 uv2;
    if (!readVertexUv(geometry, indices.x, uv0) ||
    !readVertexUv(geometry, indices.y, uv1) ||
    !readVertexUv(geometry, indices.z, uv2)) {
        return false;
    }

    vec2 barycentrics = hitResult.barycentrics;
    float w0 = 1.0 - barycentrics.x - barycentrics.y;
    uv = uv0 * w0 + uv1 * barycentrics.x + uv2 * barycentrics.y;
    return true;
}

bool reconstructHitSurface(
    RayHitResult hitResult,
    GpuSceneInstanceRecord sceneInstance,
    vec3 rayOrigin,
    vec3 rayDir,
out HitSurface surface)
{
    GpuMaterial material = BindlessMaterials[sceneInstance.bindlessMaterialIndex];
    surface = HitSurface(vec3(0.0), vec3(0.0), vec3(0.0, 0.0, 1.0), vec2(0.0), vec3(0.0), material);

    GpuGeometryRecord geometry = BindlessGeometryRecords[sceneInstance.geometryRecordIndex];

    uvec3 indices;
    if (!readTriangleVertexIndices(hitResult, geometry, indices)) {
        return false;
    }

    vec2 uv;
    if (!reconstructHitUv(hitResult, geometry, uv)) {
        return false;
    }

    vec3 objectPosition0;
    vec3 objectPosition1;
    vec3 objectPosition2;
    if (!readVertexVec3(geometry, indices.x, geometry.positionAttribOffset, objectPosition0) ||
    !readVertexVec3(geometry, indices.y, geometry.positionAttribOffset, objectPosition1) ||
    !readVertexVec3(geometry, indices.z, geometry.positionAttribOffset, objectPosition2)) {
        return false;
    }

    vec3 objectNormal0;
    vec3 objectNormal1;
    vec3 objectNormal2;
    if (!readVertexVec3(geometry, indices.x, geometry.normalAttribOffset, objectNormal0) ||
    !readVertexVec3(geometry, indices.y, geometry.normalAttribOffset, objectNormal1) ||
    !readVertexVec3(geometry, indices.z, geometry.normalAttribOffset, objectNormal2)) {
        return false;
    }

    vec2 barycentrics = hitResult.barycentrics;
    float w0 = 1.0 - barycentrics.x - barycentrics.y;
    vec3 objectPosition =
    objectPosition0 * w0 +
    objectPosition1 * barycentrics.x +
    objectPosition2 * barycentrics.y;
    vec3 objectNormal = normalize(
        objectNormal0 * w0 +
        objectNormal1 * barycentrics.x +
        objectNormal2 * barycentrics.y);
    mat3 worldToObjectLinear = mat3(
    hitResult.worldToObject[0],
    hitResult.worldToObject[1],
    hitResult.worldToObject[2]);

    surface.worldPosition = rayOrigin + rayDir * hitResult.hitT;
    surface.objectPosition = objectPosition;
    surface.worldNormal = normalize(transpose(worldToObjectLinear) * objectNormal);
    surface.uv = uv;
    surface.outgoingDirection = normalize(-rayDir);
    if (dot(surface.worldNormal, surface.outgoingDirection) < 0.0) {
        surface.worldNormal = -surface.worldNormal;
    }
    surface.material = material;
    return true;
}

#endif