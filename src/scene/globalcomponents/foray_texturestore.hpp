#pragma once
#include "../../core/foray_managedimage.hpp"
#include "../foray_component.hpp"
#include <map>

namespace foray::scene {
    class SampledTexture
    {
      public:
        std::unique_ptr<core::ManagedImage> Image;
        VkSampler                           Sampler = nullptr;
    };

    class TextureStore : public GlobalComponent
    {
      public:
        void Destroy();

        virtual ~TextureStore() { Destroy(); }

        FORAY_PROPERTY_CGET(Textures)
        FORAY_PROPERTY_GET(Textures)

        VkSampler GetOrCreateSampler(const VkSamplerCreateInfo& samplerCI);

        std::vector<VkDescriptorImageInfo> GetDescriptorInfos();

      protected:
        std::vector<SampledTexture> mTextures;

        std::map<size_t, VkSampler>        mSamplers;
    };
}  // namespace foray::scene