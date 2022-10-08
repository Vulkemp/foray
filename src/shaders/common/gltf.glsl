/*
    common/gltf.glsl

    Definitions for Material struct and material probe.

    C++: src/scenegraph/foray_material.hpp
*/

#ifndef GLTF_GLSL
#define GLTF_GLSL

/// @brief Stores properties of a material
struct MaterialBufferObject  // 52 Bytes, aligned to 16 bytes causes size to be padded to a total of 64 bytes
{
    /// @brief Base Color / Albedo Factor
    vec4 BaseColorFactor;
    /// @brief Emissive Factor
    vec3 EmissiveFactor;
    /// @brief Metallic Factor
    float MetallicFactor;
    /// @brief Roughness Factor
    float RoughnessFactor;
    /// @brief Texture Index for BaseColor
    int BaseColorTextureIndex;
    /// @brief Texture Index for MetallicRoughness
    int MetallicRoughnessTextureIndex;
    /// @brief Texture Index for Emissive
    int EmissiveTextureIndex;
    /// @brief Texture Index for Normal
    int NormalTextureIndex;
    // Index of Refraction
    float IndexOfRefraction;
    // Percentage of light transmitted through the surface
    float TransmissionFactor;
    // Texture Index for transmission factor (as multiplied with the factor)
    int TransmissionTextureIndex;
    // The color that white light turns into due to absorption when reaching the attenuation distance.
    vec3 AttenuationColor;
    // Average distance in the medium that light has to travel before it encounters a particle (world space)
    float AttenuationDistance;
    // Material Flags
    uint Flags;
};

const uint MATERIALFLAGBIT_FULLYOPAQUE = 1U;
const uint MATERIALFLAGBIT_DOUBLESIDED = 2U;

/// @brief Represents a probe of a material at a specific Uv coordinate
struct MaterialProbe
{
    /// @brief Basecolor Texture (if defined) and basecolor factor
    vec4 BaseColor;
    /// @brief Emissive Texture (if defined) and emissive factor
    vec3 EmissiveColor;
    /// @brief MetallicRoughness Texture (if defined) and metallic & roughness factor
    vec2 MetallicRoughness;
    /// @brief Normal Texture (if defined), vec3(0, 1, 0) otherwise
    vec3 Normal;
};

#endif  // GLTF_GLSL
