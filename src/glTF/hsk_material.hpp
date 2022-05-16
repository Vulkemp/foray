#pragma once
#include "../memory/hsk_vmaHelpers.hpp"
#include "glm/glm.hpp"
#include "hsk_glTF_declares.hpp"
#include "hsk_scenecomponent.hpp"
#include <tinygltf/tiny_gltf.h>
#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan.h>

namespace hsk {
    struct alignas(16) MaterialBufferObject  // 46 Bytes, aligns to 48
    {
        glm::vec4 BaseColorFactor;                // Base Color / Albedo Factor
        fp32_t    MetallicFactor;                 // Metallic Factor
        glm::vec3 EmissiveFactor;                 // Emissive Factor
        fp32_t    RoughnessFactor;                // Roughness Factor
        int16_t   BaseColorTextureIndex;          // Texture Index for BaseColor
        int16_t   MetallicRoughnessTextureIndex;  // Texture Index for MetallicRoughness
        int16_t   EmissiveTextureIndex;           // Texture Index for Emissive
        int16_t   OcclusionTextureIndex;          // Texture Index for Occlusion
        int16_t   NormalTextureIndex;             // Texture Index for Normal
    };

    struct Material : public SceneComponent
    {
      public:
        enum EAlphaMode
        {
            Opaque,
            Mask,
            Blend
        };

        Material();
        Material(Scene* scene);

        void InitFromTinyGltfMaterial(const tinygltf::Material& material);

        static EAlphaMode ParseTinyGltfAlphaMode(std::string_view raw);

        std::string Name = {};

        EAlphaMode AlphaMode     = EAlphaMode::Opaque;
        fp32_t     AlphaCutoff   = 0.5;
        bool       IsDoubleSided = false;

        glm::vec4 BaseColorFactor          = {1, 1, 1, 1};
        int32_t   BaseColorTexture         = -1;
        fp32_t    MetallicFactor           = 1;
        fp32_t    RoughnessFactor          = 1;
        int32_t   MetallicRoughnessTexture = -1;

        glm::vec3 EmissiveFactor  = glm::vec3(1.0f);
        int32_t   EmissiveTexture = -1;

        int32_t NormalTexture    = -1;
        int32_t OcclusionTexture = -1;

        MaterialBufferObject MakeBufferObject();
    };

    // @brief Manages buffer arrays containing material information
    class MaterialBuffer : public NoMoveDefaults, public SceneComponent
    {

      public:
        virtual void InitFromTinyGltfMaterials(const std::vector<tinygltf::Material>& materials);
        virtual void Cleanup();

      protected:
        std::vector<Material>             mMaterialDescriptions = {};
        std::vector<MaterialBufferObject> mBufferArray          = {};
        ManagedBuffer                     mBuffer            = {};
        VkDeviceSize                      mBufferSize           = {};

        virtual void CreateBuffer(VkDeviceSize size);
        virtual void UpdateBuffer();
        virtual void DestroyBuffer();
    };

}  // namespace hsk