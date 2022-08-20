#ifndef PAYLOAD_GLSL
#define PAYLOAD_GLSL

/// @brief Hit payload data (passed back to ray generation after ray dispatch)
struct HitPayload
{
    /// @brief Radiance in Watt per steradian per square meter
    vec3 Radiance;
    /// @brief Attenuation is a multiplier for the contribution of the current ray to its originally generated ray
    float Attenuation;
    /// @brief Seed is a per ray generated seed value for random number generation
    uint Seed;
};

#endif // PAYLOAD_GLSL