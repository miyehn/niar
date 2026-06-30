#ifndef NIAR_CSHARED_LIGHTS_GLSL
#define NIAR_CSHARED_LIGHTS_GLSL

#define MAX_LIGHTS_PER_PASS 128

#ifdef __cplusplus

#include <cstddef>
#include <glm/glm.hpp>
#define CSHARED_ALIGNAS_16 alignas(16)
namespace glm {

#else
#define CSHARED_ALIGNAS_16
#endif // #ifdef __cplusplus

struct CSHARED_ALIGNAS_16 PointLightInfo
{
    vec3 position;
    float _positionPadding;
    vec3 color;
    float _colorPadding;
};

struct CSHARED_ALIGNAS_16 DirectionalLightInfo
{
    vec3 direction;
    float _directionPadding;
    vec3 color;
    float _colorPadding;
};

#undef CSHARED_ALIGNAS_16

#ifdef __cplusplus
}; // namespace glm

static_assert(sizeof(glm::PointLightInfo) == 32);
static_assert(alignof(glm::PointLightInfo) == 16);
static_assert(offsetof(glm::PointLightInfo, position) == 0);
static_assert(offsetof(glm::PointLightInfo, color) == 16);
static_assert(sizeof(glm::DirectionalLightInfo) == 32);
static_assert(alignof(glm::DirectionalLightInfo) == 16);
static_assert(offsetof(glm::DirectionalLightInfo, direction) == 0);
static_assert(offsetof(glm::DirectionalLightInfo, color) == 16);

#endif // #ifdef __cplusplus

#endif // #ifndef NIAR_CSHARED_LIGHTS_GLSL
