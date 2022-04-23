#pragma once
#include "glm/glm.hpp"
#include "hsk_glTF_declares.hpp"
#include "hsk_scenecomponent.hpp"
#include <tinygltf/tiny_gltf.h>
#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan.h>

namespace hsk {
    struct Material : public SceneComponent
    {
      public:
        enum EAlphaMode
        {
            Opaque,
            Mask,
            Blend
        };

        struct TexCoordSets
        {
            uint8_t BaseColor         = 0;
            uint8_t MetallicRoughness = 0;
            uint8_t Normal            = 0;
            uint8_t Occlusion         = 0;
            uint8_t Emissive          = 0;
        };

        Material();
        Material(Scene* scene);

        void InitFromTinyGltfMaterial(const tinygltf::Material& material);

        static EAlphaMode ParseTinyGltfAlphaMode(std::string_view raw);

        std::string Name = {};

        EAlphaMode AlphaMode     = EAlphaMode::Opaque;
        double     AlphaCutoff   = 0.5;
        bool       IsDoubleSided = false;

        glm::vec4 BaseColorFactor          = {1, 1, 1, 1};
        Texture*  BaseColorTexture         = nullptr;
        double    MetallicFactor           = 1;
        double    RoughnessFactor          = 1;
        Texture*  MetallicRoughnessTexture = nullptr;

        glm::vec3 EmissiveFactor  = glm::vec3(1.0f);
        Texture*  EmissiveTexture = nullptr;

        Texture* NormalTexture    = nullptr;
        Texture* OcclusionTexture = nullptr;

        TexCoordSets TexCoordIndices;
    };

}  // namespace hsk