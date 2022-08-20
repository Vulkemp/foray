#ifdef BIND_TRACERCONFIG
#ifndef SET_TRACERCONFIG
#define SET_TRACERCONFIG 0
#endif // SET_TRACERCONFIG

layout(set = SET_TRACERCONFIG, binding = BIND_TRACERCONFIG) uniform TracerConfigBlock
{
    /// @brief Width and height of the output
    uvec2 Extent;
    /// @brief Per frame unique seed for random number generation
    uint RngSeed;
}
TracerConfig;

uint MakeSeed(uvec2 pixelPosition)
{
    return RngSeed * (pixelPosition.x + pixelPosition.y * Extent.x);
}

#endif // BIND_TRACERCONFIG