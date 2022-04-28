#pragma once
#include "hsk_glTF_declares.hpp"
#include "hsk_scenecomponent.hpp"

#include <tinygltf/tiny_gltf.h>
#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan.h>

namespace hsk {
    struct TextureSampler
    {
        VkFilter             MagFilter    = VkFilter::VK_FILTER_LINEAR;
        VkFilter             MinFilter    = VkFilter::VK_FILTER_LINEAR;
        VkSamplerAddressMode AddressModeU = VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_REPEAT;
        VkSamplerAddressMode AddressModeV = VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_REPEAT;
        VkSamplerAddressMode AddressModeW = VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_REPEAT;

        void InitFromTinyGltfSampler(const tinygltf::Sampler& sampler);

        static VkSamplerAddressMode sTranslateToVkWrapMode(int32_t wrapMode);
        static VkFilter             sTranslateToVkFilterMode(int32_t filterMode);
    };

    /// @brief Represents an image based texture
    class Texture : public SceneComponent, public NoMoveDefaults
    {
      public:
        inline VkImage                      Image() { return mImage; }
        inline const VkImage                Image() const { return mImage; }
        inline VkImageLayout                ImageLayout() { return mImageLayout; }
        inline const VkImageLayout          ImageLayout() const { return mImageLayout; }
        inline VkImageView                  ImageView() { return mImageView; }
        inline const VkImageView            ImageView() const { return mImageView; }
        inline VkExtent3D                   Extent() const { return mExtent; }
        inline uint32_t                     MipLevels() const { return mMipLevels; }
        inline const VkDescriptorImageInfo& DescriptorImageInfo() const { return mDescriptor; }


        Texture();
        Texture(Scene* scene);

        void InitFromTinyGltfImage(tinygltf::Image& gltfimage, TextureSampler textureSampler);
        void Cleanup();
        void UpdateDescriptor();

        inline bool IsLoaded() const { return mImage != nullptr; }

        virtual ~Texture();

      protected:
        VmaAllocation         mAllocation  = nullptr;
        VkImage               mImage       = nullptr;
        VkImageLayout         mImageLayout = {};
        VkImageView           mImageView   = nullptr;
        VkExtent3D            mExtent      = {0, 0, 1};
        uint32_t              mMipLevels   = 0;
        VkDescriptorImageInfo mDescriptor  = {};
        VkSampler             mSampler     = nullptr;
        // uint32_t              layerCount   = 0;
    };

}  // namespace hsk