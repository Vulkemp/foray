#ifndef DIFFUSEBRDF_GLSL
#define DIFFUSEBRDF_GLSL

#include "constants.glsl"

/// @brief Simple lambertian diffuse brdf
/// @remark https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#diffuse-brdf
vec3 diffuse_brdf(in vec3 rgb)
{
    return (1 / PI) * rgb;
}

#endif  // DIFFUSEBRDF_GLSL