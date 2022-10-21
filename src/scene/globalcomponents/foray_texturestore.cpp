#include "foray_texturestore.hpp"
#include "../../foray_vulkan.hpp"
#include "../../util/foray_hash.hpp"
#include <functional>
#include <spdlog/fmt/fmt.h>

namespace std {

    template <>
    struct hash<VkSamplerCreateInfo>
    {
        inline size_t operator()(const VkSamplerCreateInfo& p) const
        {
            size_t hash = 0;
            foray::util::AccumulateHash(hash, p.flags);
            foray::util::AccumulateHash(hash, p.magFilter);
            foray::util::AccumulateHash(hash, p.minFilter);
            foray::util::AccumulateHash(hash, p.mipmapMode);
            foray::util::AccumulateHash(hash, p.addressModeU);
            foray::util::AccumulateHash(hash, p.addressModeV);
            foray::util::AccumulateHash(hash, p.addressModeW);
            foray::util::AccumulateHash(hash, p.mipLodBias);
            foray::util::AccumulateHash(hash, p.anisotropyEnable);
            foray::util::AccumulateHash(hash, p.maxAnisotropy);
            foray::util::AccumulateHash(hash, p.compareEnable);
            foray::util::AccumulateHash(hash, p.compareOp);
            foray::util::AccumulateHash(hash, p.minLod);
            foray::util::AccumulateHash(hash, p.maxLod);
            foray::util::AccumulateHash(hash, p.borderColor);
            foray::util::AccumulateHash(hash, p.unnormalizedCoordinates);
            return hash;
        }
    };
}  // namespace std

namespace foray::scene {
    void TextureStore::Destroy()
    {
        mTextures.clear();
        for(auto& hashSamplerPair : mSamplers)
        {
            GetContext()->VkbDispatchTable->destroySampler(hashSamplerPair.second, nullptr);
        }
        mSamplers.clear();
    }


    VkSampler TextureStore::GetOrCreateSampler(const VkSamplerCreateInfo& samplerCI)
    {
        // hack it by
        size_t hash = std::hash<VkSamplerCreateInfo>{}(samplerCI);
        auto   find = mSamplers.find(hash);
        if(find != mSamplers.end())
        {
            return find->second;
        }
        VkSampler sampler = nullptr;
        AssertVkResult(GetContext()->VkbDispatchTable->createSampler(&samplerCI, nullptr, &sampler));

        {
            std::string                   name = fmt::format("Sampler {:x}", hash);
            VkDebugUtilsObjectNameInfoEXT nameInfo{
                .sType        = VkStructureType::VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
                .objectType   = VkObjectType::VK_OBJECT_TYPE_SAMPLER,
                .objectHandle = reinterpret_cast<uint64_t>(sampler),
                .pObjectName  = name.data(),
            };
            AssertVkResult(GetContext()->VkbDispatchTable->setDebugUtilsObjectNameEXT(&nameInfo));
        }
        mSamplers[hash] = sampler;
        return sampler;
    }

    std::shared_ptr<core::DescriptorSetHelper::DescriptorInfo> TextureStore::GetDescriptorInfo(VkShaderStageFlags shaderStage)
    {
        UpdateImageInfos();

        auto descriptorInfo = std::make_shared<core::DescriptorSetHelper::DescriptorInfo>();
        descriptorInfo->Init(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, shaderStage, &mDescriptorImageInfos);
        return descriptorInfo;
    }

    void TextureStore::UpdateImageInfos()
    {
        mDescriptorImageInfos.resize(mTextures.size());
        for(size_t i = 0; i < mTextures.size(); i++)
        {
            mDescriptorImageInfos[i].imageLayout = VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            mDescriptorImageInfos[i].imageView   = mTextures[i].Image->GetImageView();
            mDescriptorImageInfos[i].sampler     = mTextures[i].Sampler;
        }
    }

}  // namespace foray