#include "hsk_texturestore.hpp"
#include "../../hsk_vkHelpers.hpp"
#include <functional>
#include <spdlog/fmt/fmt.h>

namespace std {

    template <typename T>
    inline void accumulateHash(size_t& hash, const T& v)
    {
        // https://www.boost.org/doc/libs/1_55_0/doc/html/hash/reference.html#boost.hash_combine
        // https://www.boost.org/LICENSE_1_0.txt
        size_t vhash = std::hash<T>{}(v);
        hash ^= vhash + 0x9e3779b9 + (hash << 6) + (hash >> 2);
    }

    template <>
    struct hash<VkSamplerCreateInfo>
    {
        inline size_t operator()(const VkSamplerCreateInfo& p) const
        {
            size_t hash = 0;
            accumulateHash(hash, p.flags);
            accumulateHash(hash, p.magFilter);
            accumulateHash(hash, p.minFilter);
            accumulateHash(hash, p.mipmapMode);
            accumulateHash(hash, p.addressModeU);
            accumulateHash(hash, p.addressModeV);
            accumulateHash(hash, p.addressModeW);
            accumulateHash(hash, p.mipLodBias);
            accumulateHash(hash, p.anisotropyEnable);
            accumulateHash(hash, p.maxAnisotropy);
            accumulateHash(hash, p.compareEnable);
            accumulateHash(hash, p.compareOp);
            accumulateHash(hash, p.minLod);
            accumulateHash(hash, p.maxLod);
            accumulateHash(hash, p.borderColor);
            accumulateHash(hash, p.unnormalizedCoordinates);
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

    std::shared_ptr<DescriptorSetHelper::DescriptorInfo> TextureStore::MakeDescriptorInfo(VkShaderStageFlags shaderStage)
    {
        auto                               descriptorInfo = std::make_shared<DescriptorSetHelper::DescriptorInfo>();
        std::vector<VkDescriptorImageInfo> imageInfos;

        imageInfos.resize(mTextures.size());
        for(size_t i = 0; i < mTextures.size(); i++)
        {
            imageInfos[i].imageLayout = mTextures[i].Image->GetImageLayout();
            imageInfos[i].imageView   = mTextures[i].Image->GetImageView();
            imageInfos[i].sampler     = mTextures[i].Sampler;
        }

        descriptorInfo->Init(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, shaderStage, imageInfos);
        return descriptorInfo;
    }

}  // namespace hsk