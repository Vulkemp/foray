#pragma once
#include "../../core/managedimage.hpp"
#include "../../core/samplercollection.hpp"
#include "../../mem.hpp"
#include "../component.hpp"
#include <unordered_map>

namespace foray::scene::gcomp {
    /// @brief Manages textures and samplers
    class TextureManager : public GlobalComponent
    {
      public:
        void Destroy();

        virtual ~TextureManager() { Destroy(); }

        struct Texture
        {
          public:
            inline Texture() : mImage(), mSampler() {}
            virtual ~Texture() = default;

            void CreateImage(core::Context* context, const core::ManagedImage::CreateInfo& ci);

            FORAY_GETTER_MEM(Image)

            inline VkDescriptorImageInfo GetDescriptorImageInfo(VkImageLayout layout = VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) const
            {
                return mSampler.GetVkDescriptorInfo();
            }

            FORAY_PROPERTY_R(Sampler)

          protected:
            Local<core::ManagedImage>  mImage;
            core::CombinedImageSampler mSampler;
        };

        FORAY_GETTER_CR(Textures)
        FORAY_GETTER_MR(Textures)

        std::vector<VkDescriptorImageInfo> GetDescriptorInfos(VkImageLayout layout = VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        inline Texture& PrepareTexture(int32_t texId)
        {
            Assert(!mTextures.contains(texId));
            return mTextures[texId];
        }

      protected:
        std::unordered_map<int32_t, Texture> mTextures;
    };
}  // namespace foray::scene::gcomp