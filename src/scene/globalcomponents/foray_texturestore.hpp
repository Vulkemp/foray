#pragma once
#include "../../core/foray_managedimage.hpp"
#include "../../core/foray_samplercollection.hpp"
#include "../foray_component.hpp"
#include <unordered_map>

namespace foray::scene::gcomp {
    class TextureStore : public GlobalComponent
    {
      public:
        void Destroy();

        virtual ~TextureStore() { Destroy(); }

        struct Texture
        {
          public:
            inline Texture() : mImage(), mSampler() { mSampler.SetManagedImage(&mImage); }
            virtual ~Texture() = default;

            FORAY_PROPERTY_ALLGET(Image)
            inline VkDescriptorImageInfo GetDescriptorImageInfo(VkImageLayout layout = VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) const
            {
                return mSampler.GetVkDescriptorInfo();
            }

            FORAY_PROPERTY_ALL(Sampler)

          protected:
            core::ManagedImage         mImage;
            core::CombinedImageSampler mSampler;
        };

        FORAY_PROPERTY_ALLGET(Textures)

        std::vector<VkDescriptorImageInfo> GetDescriptorInfos(VkImageLayout layout = VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        inline Texture& PrepareTexture(int32_t texId)
        {
            Assert(!mTextures.contains(texId));
            return mTextures[texId];
        }

      protected:
        std::unordered_map<int32_t, Texture> mTextures;
    };
}  // namespace foray::scene