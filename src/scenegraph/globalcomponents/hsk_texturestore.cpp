#include "hsk_texturestore.hpp"
#include "../../hsk_vkHelpers.hpp"
#include "../../utility/hsk_hash.hpp"
#include <functional>
#include <spdlog/fmt/fmt.h>

namespace std {

    template <>
    struct hash<VkSamplerCreateInfo>
    {
        inline size_t operator()(const VkSamplerCreateInfo& p) const
        {
            size_t hash = 0;
            hsk::AccumulateHash(hash, p.flags);
            hsk::AccumulateHash(hash, p.magFilter);
            hsk::AccumulateHash(hash, p.minFilter);
            hsk::AccumulateHash(hash, p.mipmapMode);
            hsk::AccumulateHash(hash, p.addressModeU);
            hsk::AccumulateHash(hash, p.addressModeV);
            hsk::AccumulateHash(hash, p.addressModeW);
            hsk::AccumulateHash(hash, p.mipLodBias);
            hsk::AccumulateHash(hash, p.anisotropyEnable);
            hsk::AccumulateHash(hash, p.maxAnisotropy);
            hsk::AccumulateHash(hash, p.compareEnable);
            hsk::AccumulateHash(hash, p.compareOp);
            hsk::AccumulateHash(hash, p.minLod);
            hsk::AccumulateHash(hash, p.maxLod);
            hsk::AccumulateHash(hash, p.borderColor);
            hsk::AccumulateHash(hash, p.unnormalizedCoordinates);
            return hash;
        }
    };
}  // namespace std

namespace hsk {
    void TextureStore::Cleanup()
    {
        mTextures.clear();
        for(auto& hashSamplerPair : mSamplers)
        {
            vkDestroySampler(GetContext()->Device, hashSamplerPair.second, nullptr);
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
        AssertVkResult(vkCreateSampler(GetContext()->Device, &samplerCI, nullptr, &sampler));
        if(GetContext()->DebugEnabled)
        {
            std::string                   name = fmt::format("Sampler {:x}", hash);
            VkDebugUtilsObjectNameInfoEXT nameInfo{
                .sType        = VkStructureType::VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
                .objectType   = VkObjectType::VK_OBJECT_TYPE_SAMPLER,
                .objectHandle = reinterpret_cast<uint64_t>(sampler),
                .pObjectName  = name.data(),
            };
            AssertVkResult(GetContext()->DispatchTable.setDebugUtilsObjectNameEXT(&nameInfo));
        }
        mSamplers[hash] = sampler;
        return sampler;
    }

    std::shared_ptr<DescriptorSetHelper::DescriptorInfo> TextureStore::GetDescriptorInfo(VkShaderStageFlags shaderStage)
    {
        UpdateImageInfos();

        auto descriptorInfo = std::make_shared<DescriptorSetHelper::DescriptorInfo>();
        descriptorInfo->Init(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, shaderStage, &mDescriptorImageInfos);
        return descriptorInfo;
    }

    void TextureStore::UpdateImageInfos()
    {
        mDescriptorImageInfos.resize(mTextures.size());
        for(size_t i = 0; i < mTextures.size(); i++)
        {
            mDescriptorImageInfos[i].imageLayout = mTextures[i].Image->GetImageLayout();
            mDescriptorImageInfos[i].imageView   = mTextures[i].Image->GetImageView();
            mDescriptorImageInfos[i].sampler     = mTextures[i].Sampler;
        }
    }

}  // namespace hsk