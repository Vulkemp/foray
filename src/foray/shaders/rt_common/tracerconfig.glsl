/*
    rt_common/tracerconfig.glsl

    Layout macro for TracerConfiguration Ubo
*/

#ifdef PUSHC_TRACERCONFIG
/// @brief Configuration parameters for raytracing
layout(push_constant) uniform TracerConfigBlock
{
    /// @brief Per frame unique seed for random number generation
    uint RngSeed;
}
TracerConfig;
#endif // PUSHC_TRACERCONFIG