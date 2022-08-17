#pragma once
#include "../../memory/hsk_descriptorsethelper.hpp"
#include "../../memory/hsk_managedimage.hpp"
#include "../hsk_component.hpp"
#include <map>

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
        void Destroy();

        virtual ~TextureStore() { Destroy(); }

        HSK_PROPERTY_CGET(Textures)
        HSK_PROPERTY_GET(Textures)

        VkSampler GetOrCreateSampler(const VkSamplerCreateInfo& samplerCI);

        std::shared_ptr<DescriptorSetHelper::DescriptorInfo> GetDescriptorInfo(VkShaderStageFlags shaderStage = VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT);

      protected:
        std::vector<SampledTexture> mTextures;
        std::map<size_t, VkSampler> mSamplers;
        std::vector<VkDescriptorImageInfo>                   mDescriptorImageInfos;
        void                                                 UpdateImageInfos();
    };
}  // namespace hsk