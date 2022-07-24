#ifndef ENV_MAP_GLSL
#define ENV_MAP_GLSL

#ifdef BIND_ENVMAP_CUBESAMPLER
#ifndef SET_ENVMAP_CUBESAMPLER
#define SET_ENVMAP_CUBESAMPLER 0
#endif  // SET_ENVMAP_CUBESAMPLER
layout(set = SET_ENVMAP_CUBESAMPLER, binding = BIND_ENVMAP_CUBESAMPLER) samplerCube EnvironmentMap;
#endif  // BIND_ENVMAP_CUBESAMPLER

#ifdef BIND_ENVMAP_SPHERESAMPLER
#ifndef SET_ENVMAP_SPHERESAMPLER
#define SET_ENVMAP_SPHERESAMPLER 0
#endif  // SET_ENVMAP_SPHERESAMPLER
layout(set = SET_ENVMAP_SPHERESAMPLER, binding = BIND_ENVMAP_SPHERESAMPLER) sampler2D EnvironmentMap;

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

vec4 SampleEnvironmentMap(vec3 dir)
{
#ifdef BIND_ENVMAP_CUBESAMPLER
    return texture(EnvironmentMap, dir);
#endif
#ifdef BIND_ENVMAP_SPHERESAMPLER
    return texture(EnvironmentMap, SampleSphericalMap(dir));
#endif
    return vec4(1.f);
}

#endif