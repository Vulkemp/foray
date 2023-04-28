/*
    rt_common/tracerconfig.glsl

    Layout macro for TracerConfiguration Ubo
*/

#ifdef PUSHC_TRACERCONFIG

#ifdef PUSH_CONSTANTS_DEFINED
#error "Push constants were already defined!"
#endif
#define PUSH_CONSTANTS_DEFINED

/// @brief Configuration parameters for raytracing
layout(push_constant) uniform TracerConfigBlock
{
    /// @brief Per frame unique seed for random number generation
    uint RngSeed;
}
TracerConfig;
#endif // PUSHC_TRACERCONFIG