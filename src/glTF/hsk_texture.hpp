#pragma once
#include "hsk_glTF_declares.hpp"
#include <tinygltf/tiny_gltf.h>
#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan.h>

namespace hsk {
    struct TextureSampler
    {
        VkFilter             MagFilter;
        VkFilter             MinFilter;
        VkSamplerAddressMode AddressModeU;
        VkSamplerAddressMode AddressModeV;
        VkSamplerAddressMode AddressModeW;
    };

    class Texture : public NoMoveDefaults
    {
      public:
        Texture();
        Texture(Scene* scene);

        inline Texture& OwningScene(hsk::Scene* scene)
        {
            mOwningScene = scene;
            return *this;
        }
        inline hsk::Scene*                  OwningScene() { return mOwningScene; }
        inline const hsk::Scene*            OwningScene() const { return mOwningScene; }
        inline VkImage                      Image() { return mImage; }
        inline const VkImage                Image() const { return mImage; }
        inline VkImageLayout                ImageLayout() { return mImageLayout; }
        inline const VkImageLayout          ImageLayout() const { return mImageLayout; }
        inline VkImageView                  ImageView() { return mImageView; }
        inline const VkImageView            ImageView() const { return mImageView; }
        inline VkExtent3D                   Extent() const { return mExtent; }
        inline uint32_t                     MipLevels() const { return mMipLevels; }
        inline const VkDescriptorImageInfo& DescriptorImageInfo() const { return mDescriptor; }


        void InitFromTinyGltfImage(tinygltf::Image& gltfimage, TextureSampler textureSampler);
        void Cleanup();
        void UpdateDescriptor();

        inline bool IsLoaded() const { return mImage != nullptr; }


        virtual ~Texture();

      protected:
        hsk::Scene*           mOwningScene = nullptr;
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