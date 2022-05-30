#pragma once
#include "../memory/hsk_managedbuffer.hpp"
#include "glm/glm.hpp"
#include "hsk_glTF_declares.hpp"
#include "hsk_scenecomponent.hpp"
#include <tinygltf/tiny_gltf.h>
#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan.h>

namespace hsk {
    struct alignas(16) MaterialBufferObject  // 52 Bytes, aligned to 16 bytes causes size to be padded to a total of 64 bytes
    {
        glm::vec4 BaseColorFactor;                // Base Color / Albedo Factor
        fp32_t    MetallicFactor;                 // Metallic Factor
        glm::vec3 EmissiveFactor;                 // Emissive Factor
        fp32_t    RoughnessFactor;                // Roughness Factor
        int32_t   BaseColorTextureIndex;          // Texture Index for BaseColor
        int32_t   MetallicRoughnessTextureIndex;  // Texture Index for MetallicRoughness
        int32_t   EmissiveTextureIndex;           // Texture Index for Emissive
        int32_t   NormalTextureIndex;             // Texture Index for Normal
    };

    struct Material
    {
      public:
        enum EAlphaMode
        {
            Opaque,
            Mask,
            Blend
        };

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

        MaterialBufferObject MakeBufferObject();
    };

    // @brief Manages buffer arrays containing material information
    class MaterialBuffer : public NoMoveDefaults, public SceneComponent
    {
      public:
        inline MaterialBuffer() {}
        inline explicit MaterialBuffer(Scene* scene) : SceneComponent(scene){}

        virtual void InitFromTinyGltfMaterials(const std::vector<tinygltf::Material>& materials);
        virtual void UpdateBuffer();
        virtual void Cleanup();

        HSK_PROPERTY_GET(MaterialDescriptions)
        HSK_PROPERTY_CGET(MaterialDescriptions)
        HSK_PROPERTY_GET(BufferArray)
        HSK_PROPERTY_CGET(BufferArray)
        HSK_PROPERTY_GET(Buffer)
        HSK_PROPERTY_CGET(Buffer)
        HSK_PROPERTY_GET(BufferSize)
        HSK_PROPERTY_CGET(BufferSize)
        HSK_PROPERTY_GET(BufferCapacity)
        HSK_PROPERTY_CGET(BufferCapacity)

      protected:
        std::vector<Material>             mMaterialDescriptions = {};
        std::vector<MaterialBufferObject> mBufferArray          = {};
        ManagedBuffer                     mBuffer            = {};
        VkDeviceSize                      mBufferSize           = {};
        VkDeviceSize                      mBufferCapacity           = {};

        virtual void CreateBuffer();
        virtual void DestroyBuffer();

        virtual void WriteDescriptorSet(VkDescriptorSet set, uint32_t binding);
    };

}  // namespace hsk