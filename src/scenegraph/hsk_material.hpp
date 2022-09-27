#pragma once
#include "glm/glm.hpp"
#include "hsk_scenegraph_declares.hpp"

namespace hsk {

    struct alignas(16) MaterialBufferBlock
    {
        uint8_t Values[128];  // 128 bytes for material definition
    };

    class MaterialMeta
    {
      public:
        uint32_t DefaultShaderBindingTableOffset = 0;
    };

    class MaterialBase
    {
      public:
        virtual void                WriteToBufferBlock(MaterialBufferBlock& block) = 0;
        virtual const MaterialMeta& GetMaterialMeta() const                        = 0;
        virtual void                ImGuiEdit() {}
    };

    /// @brief Represents the default gltf pbr material type, capable of representing opaque surfaces with a metallic/roughness model including emission
    struct alignas(16) DefaultMaterialEntry
    {
        glm::vec4 BaseColorFactor;                // Base Color / Albedo Factor
        glm::vec3 EmissiveFactor;                 // Emissive Factor
        fp32_t    MetallicFactor;                 // Metallic Factor
        fp32_t    RoughnessFactor;                // Roughness Factor
        int32_t   BaseColorTextureIndex;          // Texture Index for BaseColor
        int32_t   MetallicRoughnessTextureIndex;  // Texture Index for MetallicRoughness
        int32_t   EmissiveTextureIndex;           // Texture Index for Emissive
        int32_t   NormalTextureIndex;             // Texture Index for Normal
    };

    /// @brief Represents the gltf transmissive material type, interpreting the geometry as infinitely small volume walls
    struct alignas(16) TransmissiveMaterialEntry
    {
        glm::vec4 BaseColorFactor;                // Base Color / Albedo Factor
        fp32_t    MetallicFactor;                 // Metallic Factor
        fp32_t    RoughnessFactor;                // Roughness Factor
        int32_t   BaseColorTextureIndex;          // Texture Index for BaseColor
        int32_t   MetallicRoughnessTextureIndex;  // Texture Index for MetallicRoughness
        int32_t   NormalTextureIndex;             // Texture Index for Normal
        fp32_t    IndexOfRefraction;              // 
        fp32_t    TransmissionFactor;             // Percentage of light transmitted through the surface
        int32_t   TransmissionTexture;            // Texture Index for transmission factor (as multiplied with the factor)
    };

    /// @brief Represents the gltf volume material type, interpreting the geometry as the boundary of a volume
    struct alignas(16) VolumeMaterialEntry
    {
        glm::vec4 BaseColorFactor;                // Base Color / Albedo Factor
        fp32_t    MetallicFactor;                 // Metallic Factor
        fp32_t    RoughnessFactor;                // Roughness Factor
        int32_t   BaseColorTextureIndex;          // Texture Index for BaseColor
        int32_t   MetallicRoughnessTextureIndex;  // Texture Index for MetallicRoughness
        int32_t   NormalTextureIndex;             // Texture Index for Normal
        fp32_t    IndexOfRefraction;              // 
        fp32_t    TransmissionFactor;             // Percentage of light transmitted through the surface
        int32_t   TransmissionTexture;            // Texture Index for transmission factor (as multiplied with the factor)
    };

    class DefaultMaterial : public MaterialBase
    {
      public:
        inline static constexpr MaterialMeta sMeta = MaterialMeta{.DefaultShaderBindingTableOffset = 0};

        DefaultMaterialEntry Data;

        virtual void                WriteToBufferBlock(MaterialBufferBlock& block) override;
        virtual const MaterialMeta& GetMaterialMeta() const override;
    };
}  // namespace hsk