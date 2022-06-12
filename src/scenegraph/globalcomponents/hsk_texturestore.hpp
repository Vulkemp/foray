#pragma once
#include "../../memory/hsk_managedimage.hpp"
#include "../hsk_component.hpp"

namespace hsk {
    class SampledTexture
    {
      public:
        std::unique_ptr<ManagedImage> Image;
        VkSampler                     Sampler;
    };

    class TextureStore : public GlobalComponent
    {
      public:
        void Cleanup();

        virtual ~TextureStore() { Cleanup(); }

        HSK_PROPERTY_CGET(Textures)
        HSK_PROPERTY_GET(Textures)

        VkSampler GetOrCreateSampler(const VkSamplerCreateInfo& samplerCI);

        std::shared_ptr<DescriptorSetHelper::DescriptorInfo> MakeDescriptorInfo(VkShaderStageFlags shaderStage = VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT);

      protected:
        std::vector<SampledTexture> mTextures;
        std::map<size_t, VkSampler> mSamplers;
    };
}  // namespace hsk