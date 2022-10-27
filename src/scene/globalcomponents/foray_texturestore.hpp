#pragma once
#include "../../core/foray_descriptorsethelper.hpp"
#include "../../core/foray_managedimage.hpp"
#include "../foray_component.hpp"
#include <map>

namespace foray::scene {
    class SampledTexture
    {
      public:
        std::unique_ptr<core::ManagedImage> Image;
        VkSampler                     Sampler = nullptr;
    };

    class TextureStore : public GlobalComponent
    {
      public:
        void Destroy();

        virtual ~TextureStore() { Destroy(); }

        FORAY_PROPERTY_CGET(Textures)
        FORAY_PROPERTY_GET(Textures)

        VkSampler GetOrCreateSampler(const VkSamplerCreateInfo& samplerCI);

        std::shared_ptr<core::DescriptorSetHelper::DescriptorInfo> GetDescriptorInfo(VkShaderStageFlags shaderStage = VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT);
        std::vector<VkDescriptorImageInfo>&                        GetDescriptorImageInfos();
      protected:
        std::vector<SampledTexture> mTextures;

        std::map<size_t, VkSampler>        mSamplers;
        std::vector<VkDescriptorImageInfo> mDescriptorImageInfos;
        void                               UpdateImageInfos();
    };
}  // namespace foray