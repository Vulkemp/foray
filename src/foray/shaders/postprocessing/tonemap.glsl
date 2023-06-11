#include "tonemap_aces.glsl"
#include "tonemap_amd.glsl"

// Inspired by https://github.com/GPUOpen-LibrariesAndSDKs/Cauldron/blob/master/src/VK/shaders/tonemapping.glsl
vec3 ApplyTonemap(uint tonemapIdx, vec3 color, float exposure)
{
    color *= exposure;

    switch (tonemapIdx)
    {
        case 0: return color;
        case 1: return AcesTonemapper(color);
        case 2: return AMDTonemapper(color);
    }
    return color;
}