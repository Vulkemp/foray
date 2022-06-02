#pragma once
#include "hsk_gltf_declares.hpp"
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
        HSK_PROPERTY_GET(Image);
        HSK_PROPERTY_GET(ImageLayout);
        HSK_PROPERTY_GET(ImageView);
        HSK_PROPERTY_GET(Extent);
        HSK_PROPERTY_GET(MipLevels);
        HSK_PROPERTY_GET(Descriptor);
        HSK_PROPERTY_GET(Sampler);
        HSK_PROPERTY_CGET(Image);
        HSK_PROPERTY_CGET(ImageLayout);
        HSK_PROPERTY_CGET(ImageView);
        HSK_PROPERTY_CGET(Extent);
        HSK_PROPERTY_CGET(MipLevels);
        HSK_PROPERTY_CGET(Descriptor);
        HSK_PROPERTY_CGET(Sampler);

        Texture(Scene* scene);

        void InitFromTinyGltfImage(tinygltf::Image& gltfimage, TextureSampler textureSampler);
        void Cleanup();
        void UpdateDescriptor();

        inline bool IsLoaded() const { return mImage != nullptr; }

        virtual ~Texture();

      protected:
        int32_t               mIndex       = 0;
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