#ifndef ENV_MAP_GLSL
#define ENV_MAP_GLSL

#ifdef BIND_ENVMAP_CUBESAMPLER
#ifndef SET_ENVMAP_CUBESAMPLER
#define SET_ENVMAP_CUBESAMPLER 0
#endif  // SET_ENVMAP_CUBESAMPLER
layout(set = SET_ENVMAP_CUBESAMPLER, binding = BIND_ENVMAP_CUBESAMPLER) samplerCube EnvironmentCube;
#endif  // BIND_ENVMAP_CUBESAMPLER

#ifdef BIND_ENVMAP_SPHERESAMPLER
#ifndef SET_ENVMAP_SPHERESAMPLER
#define SET_ENVMAP_SPHERESAMPLER 0
#endif  // SET_ENVMAP_SPHERESAMPLER
layout(set = SET_ENVMAP_SPHERESAMPLER, binding = BIND_ENVMAP_SPHERESAMPLER) sampler2D EnvironmentSphere;

// https://learnopengl.com/PBR/IBL/Diffuse-irradiance CC BY 4.0 https://learnopengl.com/About
const vec2 invAtan = vec2(0.1591, 0.3183);
vec2       SampleSphericalMap(vec3 v)
{
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= invAtan;
    uv += 0.5;
    return uv;
}

#endif  // BIND_ENVMAP_SPHERESAMPLER

#ifdef BIND_ENVMAP_UBO
#ifndef SET_ENVMAP_UBO
#define SET_ENVMAP_UBO 0
#endif
layout(set = SET_ENVMAP_UBO, binding = BIND_ENVMAP_UBO, std430) uniform
{
    bool UseCubemap;
    bool UseSpheremap;
}
EnvmapConfig;

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