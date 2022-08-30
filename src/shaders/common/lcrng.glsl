/*
    common/lcgrng.glsl
*/

#ifndef LCGRNG_GLSL
#define LCGRNG_GLSL


// Placeholder values https://en.wikipedia.org/wiki/Linear_congruential_generator, same values as used by glibc / GCC
// TODO: consider other values (shader limited to 32bit atm!), for example from https://github.com/vigna/CPRNG

const uint LCGRAND_MULTIPLIER = 1664525u;
const uint LCGRAND_INCREMENT = 1013904223u;
const uint LCGRAND_MASK = 0x00FFFFFFu;

const uint LCGRAND_MIN = 0x0u;
const uint LCGRAND_MAX = LCGRAND_MASK - 1;

uint lcgUint(inout uint seed)
{
    seed = (LCGRAND_MULTIPLIER * seed + LCGRAND_INCREMENT) & LCGRAND_MASK;
    return seed;
}

uvec2 lcgUvec2(inout uint seed)
{
    return uvec2(lcgUint(seed), lcgUint(seed));
}

uvec3 lcgUvec3(inout uint seed)
{
    return uvec3(lcgUint(seed), lcgUint(seed), lcgUint(seed));
}

float lcgFloat(inout uint seed)
{
    return float(lcgUint(seed)) / float(LCGRAND_MAX);
}

vec2 lcgVec2(inout uint seed)
{
    return vec2(lcgFloat(seed), lcgFloat(seed));
}

vec3 lcgVec3(inout uint seed)
{
    return vec3(lcgFloat(seed), lcgFloat(seed), lcgFloat(seed));
}

#endif // LCGRNG_GLSL