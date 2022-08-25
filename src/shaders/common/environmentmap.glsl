/*
    common/environmentmap.glsl

    Contains layout macros for cube and sphere projected environmentmaps aswell as 
    functions for sampling both projection types through a shared method.
*/

#ifndef ENV_MAP_GLSL
#define ENV_MAP_GLSL

#ifdef BIND_ENVMAP_CUBESAMPLER
#ifndef SET_ENVMAP_CUBESAMPLER
#define SET_ENVMAP_CUBESAMPLER 0
#endif  // SET_ENVMAP_CUBESAMPLER
layout(set = SET_ENVMAP_CUBESAMPLER, binding = BIND_ENVMAP_CUBESAMPLER) uniform samplerCube EnvironmentCube;
#endif  // BIND_ENVMAP_CUBESAMPLER

#ifdef BIND_ENVMAP_SPHERESAMPLER
#ifndef SET_ENVMAP_SPHERESAMPLER
#define SET_ENVMAP_SPHERESAMPLER 0
#endif  // SET_ENVMAP_SPHERESAMPLER
layout(set = SET_ENVMAP_SPHERESAMPLER, binding = BIND_ENVMAP_SPHERESAMPLER) uniform sampler2D EnvironmentSphere;

// https://learnopengl.com/PBR/IBL/Diffuse-irradiance CC BY 4.0 https://learnopengl.com/About
const vec2 invAtan = vec2(0.1591, 0.3183);

/// @brief Calculates uv coordinates for sampling a spherical environment map
/// @param dir Directional vector
vec2 SampleSphericalMap(vec3 dir)
{
    vec2 uv = vec2(atan(dir.z, dir.x), asin(dir.y));
    uv *= invAtan;
    uv += 0.5;
    return uv;
}

#endif  // BIND_ENVMAP_SPHERESAMPLER

#ifdef BIND_ENVMAP_UBO
#ifndef SET_ENVMAP_UBO
#define SET_ENVMAP_UBO 0
#endif
/// @brief Ubo defining wether cube and/or sphere map are bound
layout(set = SET_ENVMAP_UBO, binding = BIND_ENVMAP_UBO, std430) uniform EnvmapConfigBlock
{
    bool UseCubemap;
    bool UseSpheremap;
}
EnvmapConfig;

/// @brief Automatically sample the correct environmentmap
vec4 SampleEnvironmentMap(vec3 dir)
{
    if(EnvmapConfig.UseCubemap)
    {
        return texture(EnvironmentCube, dir);
    }
    if(EnvmapConfig.UseSpheremap)
    {
        return texture(EnvironmentSphere, SampleSphericalMap(dir));
    }
    return vec4(0.f);
}

#else

/// @brief Automatically sample the correct environmentmap
vec4 SampleEnvironmentMap(vec3 dir)
{
#ifdef BIND_ENVMAP_CUBESAMPLER
    return texture(EnvironmentCube, dir);
#endif
#ifdef BIND_ENVMAP_SPHERESAMPLER
    return texture(EnvironmentSphere, SampleSphericalMap(dir));
#endif
}

#endif
#endif