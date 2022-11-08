#ifdef BIND_NOISETEX
#ifndef SET_NOISETEX
#define SET_NOISETEX 0
#endif // SET_NOISETEX
layout(r32ui, set = SET_NOISETEX, binding = BIND_NOISETEX) uniform uimage2D NoiseSource;
#endif // BIND_NOISETEX