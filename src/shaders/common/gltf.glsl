/*
    common/gltf.glsl

    Definitions for Material struct and material probe.

    C++: src/scenegraph/hsk_material.hpp
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
};

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
