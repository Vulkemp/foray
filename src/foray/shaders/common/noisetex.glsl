#ifdef BIND_NOISETEX

#include "lcrng.glsl"

#ifndef SET_NOISETEX
#define SET_NOISETEX 0
#endif // SET_NOISETEX
layout(r32ui, set = SET_NOISETEX, binding = BIND_NOISETEX) uniform readonly uimage2D NoiseSource;

uint CalculateSeed(ivec2 texelPos, uint seed)
{
    ivec2 size = imageSize(NoiseSource);
    uint left = imageLoad(NoiseSource, ivec2(texelPos.x % size.x, texelPos.y % size.y)).r;
    uint right = ~0U - seed;

    for (int i = 0; i < 8; i++)
    {
        uint temp = left;
        left += lcgUint(temp) + right;
        temp = right;
        right += lcgUint(temp) + left;
        left += temp;
        temp = left;
        left = right;
        right = temp;
    }

    return left + right;
}

#endif // BIND_NOISETEX