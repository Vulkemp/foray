/*
    common/random.glsl
*/

#ifndef RANDOM_GLSL
#define RANDOM_GLSL


// Placeholder values https://en.wikipedia.org/wiki/Linear_congruential_generator, same values as used by glibc / GCC
// TODO: consider other values (shader limited to 32bit atm!), for example from https://github.com/vigna/CPRNG

const uint LCGRAND_MULTIPLIER = 0x41C64E6Du;
const uint LCGRAND_INCREMENT = 0x3039u;
const uint LCGRAND_MASK = 0x7FFFFFFFu;

const uint LCGRAND_MIN = 0x0u;
const uint LCGRAND_MAX = LCGRAND_MASK - 1;

uint RandomUint(inout uint seed)
{
    seed = (LCGRAND_MULTIPLIER * seed + LCGRAND_INCREMENT) & LCGRAND_MASK;
    return seed;
}

uvec2 RandomUvec2(inout uint seed)
{
    return uvec2(RandomUint(seed), RandomUint(seed));
}

uvec3 RandomUvec3(inout uint seed)
{
    return uvec3(RandomUint(seed), RandomUint(seed), RandomUint(seed));
}

float RandomFp32(inout uint seed)
{
    return float(RandomUint(seed)) / float(LCGRAND_MAX);
}

vec2 RandomVec2(inout uint seed)
{
    return vec2(RandomFp32(seed), RandomFp32(seed));
}

vec3 RandomVec3(inout uint seed)
{
    return vec3(RandomFp32(seed), RandomFp32(seed), RandomFp32(seed));
}

#endif // RANDOM_GLSL