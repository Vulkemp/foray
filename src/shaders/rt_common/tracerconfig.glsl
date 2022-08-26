/*
    rt_common/tracerconfig.glsl

    Layout macro for TracerConfiguration Ubo
*/

#ifdef BIND_TRACERCONFIG
#ifndef SET_TRACERCONFIG
#define SET_TRACERCONFIG 0
#endif // SET_TRACERCONFIG

/// @brief Configuration parameters for raytracing
layout(set = SET_TRACERCONFIG, binding = BIND_TRACERCONFIG) uniform TracerConfigBlock
{
    /// @brief Per frame unique seed for random number generation
    uint RngSeed;
}
TracerConfig;

uint MakeSeed(uvec2 pixelPosition)
{
    return TracerConfig.RngSeed * (pixelPosition.x + pixelPosition.y * Extent.x);
}

#endif // BIND_TRACERCONFIG