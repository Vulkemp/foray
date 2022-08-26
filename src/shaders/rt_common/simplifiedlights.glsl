#ifndef SIMPLIFIED_LIGHTS_GLSL
#define SIMPLIFIED_LIGHTS_GLSL

const uint SimplifiedLightType_Directional = 0;
const uint SimplifiedLightType_Point       = 1;

/// @brief Describes a simplified light source
struct SimplifiedLight  // std430
{
    /// @brief Radiant flux in Watt per primary wavelength
    vec3 RadiantFluxRgb;
    /// @brief Type (see SimplifiedLightType enum values)
    uint Type;
    /// @brief Position or direction, interprete based on type
    vec3 PosOrDir;
};

#endif  // SIMPLIFIED_LIGHTS_GLSL

#ifdef BIND_SIMPLIFIEDLIGHTARRAY
#ifndef SET_SIMPLIFIEDLIGHTARRAY
#define SET_SIMPLIFIEDLIGHTARRAY 0
#endif  // SET_SIMPLIFIEDLIGHTARRAY
/// @brief Buffer containing array of simplified lights
layout(set = SET_SIMPLIFIEDLIGHTARRAY, binding = BIND_SIMPLIFIEDLIGHTARRAY, std430) buffer readonly SimplifiedLightsBuffer
{
    /// @brief Array of simplifiedlight structures (guaranteed at minimum Count)
    SimplifiedLight Array[];
    /// @brief Count of lights enabled
    uint Count;
}
SimplifiedLights;
#endif  // BIND_SIMPLIFIEDLIGHTARRAY
