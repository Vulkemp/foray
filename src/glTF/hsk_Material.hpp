#pragma once
#include "glm/glm.hpp"
#include "hsk_glTF_declares.hpp"
#include <tinygltf/tiny_gltf.h>
#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan.h>

namespace hsk {
    struct Material
    {
        enum AlphaMode
        {
            ALPHAMODE_OPAQUE,
            ALPHAMODE_MASK,
            ALPHAMODE_BLEND
        };
        AlphaMode alphaMode       = ALPHAMODE_OPAQUE;
        float     alphaCutoff     = 1.0f;
        float     metallicFactor  = 1.0f;
        float     roughnessFactor = 1.0f;
        glm::vec4 baseColorFactor = glm::vec4(1.0f);
        glm::vec4 emissiveFactor  = glm::vec4(1.0f);
        Texture*  baseColorTexture;
        Texture*  metallicRoughnessTexture;
        Texture*  normalTexture;
        Texture*  occlusionTexture;
        Texture*  emissiveTexture;
        struct TexCoordSets
        {
            uint8_t baseColor          = 0;
            uint8_t metallicRoughness  = 0;
            uint8_t specularGlossiness = 0;
            uint8_t normal             = 0;
            uint8_t occlusion          = 0;
            uint8_t emissive           = 0;
        } texCoordSets;
        struct Extension
        {
            Texture*  specularGlossinessTexture;
            Texture*  diffuseTexture;
            glm::vec4 diffuseFactor  = glm::vec4(1.0f);
            glm::vec3 specularFactor = glm::vec3(0.0f);
        } extension;
        struct PbrWorkflows
        {
            bool metallicRoughness  = true;
            bool specularGlossiness = false;
        } pbrWorkflows;
        VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
    };

}  // namespace hsk