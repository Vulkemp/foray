#include "foray_samplercollection.hpp"
#include "../util/foray_hash.hpp"
#include "foray_context.hpp"

namespace foray::core {
    CombinedImageSampler::CombinedImageSampler(const CombinedImageSampler& other)
        : mManagedImage(other.mManagedImage), mSampler(other.mSampler), mHash(other.mHash), mCollection(other.mCollection)
    {
        if(!!mSampler && !!mCollection)
        {
            mCollection->Register(*this);
        }
    }
    CombinedImageSampler::CombinedImageSampler(CombinedImageSampler&& other)
        : mManagedImage(other.mManagedImage), mSampler(other.mSampler), mHash(other.mHash), mCollection(other.mCollection)
    {
        if(!!mSampler && !!mCollection)
        {
            mCollection->Register(*this);
        }
    }
    CombinedImageSampler& CombinedImageSampler::operator=(const CombinedImageSampler& other)
    {
        Destroy();
        mManagedImage = other.mManagedImage;
        mSampler      = other.mSampler;
        mHash         = other.mHash;
        mCollection   = other.mCollection;
        if(!!mSampler && !!mCollection)
        {
            mCollection->Register(*this);
        }
        return *this;
    }
    CombinedImageSampler::~CombinedImageSampler()
    {
        Destroy();
    }
    void CombinedImageSampler::Destroy()
    {
        if(!!mSampler && !!mCollection)
        {
            mCollection->Unregister(*this);
            mSampler    = nullptr;
            mCollection = nullptr;
        }
    }
    CombinedImageSampler::CombinedImageSampler(core::Context* context, core::ManagedImage* image, const VkSamplerCreateInfo& samplerCi)
    {
        mManagedImage = image;
        mCollection   = context->SamplerCol;
        if(!!mCollection)
        {
            mCollection->GetSampler(*this, samplerCi);
        }
    }
    void CombinedImageSampler::Init(core::Context* context, const VkSamplerCreateInfo& samplerCi)
    {
        Destroy();
        mCollection   = context->SamplerCol;
        if(!!mCollection)
        {
            mCollection->GetSampler(*this, samplerCi);
        }
    }
    void CombinedImageSampler::Init(core::Context* context, core::ManagedImage* image, const VkSamplerCreateInfo& samplerCi)
    {
        Destroy();
        mManagedImage = image;
        mCollection   = context->SamplerCol;
        if(!!mCollection)
        {
            mCollection->GetSampler(*this, samplerCi);
        }
    }

    void SamplerCollection::GetSampler(CombinedImageSampler& imageSampler, const VkSamplerCreateInfo& samplerCi)
    {
        uint64_t hash = GetHash(samplerCi);
        auto     iter = mSamplerInstances.find(hash);
        if(iter != mSamplerInstances.end())
        {
            SamplerInstance& instance = iter->second;
            instance.RefCount++;
            imageSampler.mHash    = hash;
            imageSampler.mSampler = instance.Sampler;
        }
        else
        {
            SamplerInstance instance{.SamplerHash = hash, .RefCount = 1};
            AssertVkResult(mContext->VkbDispatchTable->createSampler(&samplerCi, nullptr, &(instance.Sampler)));
            mSamplerInstances[hash] = instance;
            imageSampler.mHash      = hash;
            imageSampler.mSampler   = instance.Sampler;
        }
    }
    void SamplerCollection::Register(CombinedImageSampler& imageSampler)
    {
        mSamplerInstances[imageSampler.mHash].RefCount++;
    }
    void SamplerCollection::Unregister(CombinedImageSampler& imageSampler)
    {
        auto iter = mSamplerInstances.find(imageSampler.mHash);
        if(iter != mSamplerInstances.end())
        {
            SamplerInstance& instance = iter->second;
            instance.RefCount--;
            if(instance.RefCount == 0)
            {
                mContext->VkbDispatchTable->destroySampler(instance.Sampler, nullptr);
                mSamplerInstances.erase(iter);
            }
        }
    }
    void SamplerCollection::ClearSamplerMap()
    {
        for(auto pair : mSamplerInstances)
        {
            SamplerInstance& instance = pair.second;
            mContext->VkbDispatchTable->destroySampler(instance.Sampler, nullptr);
        }
        mSamplerInstances.clear();
    }
    uint64_t SamplerCollection::GetHash(const VkSamplerCreateInfo& samplerCi)
    {
        Assert(samplerCi.pNext == nullptr, "Cannot hash .pNext values!");
        size_t hash = 0;
        util::AccumulateHash(hash, samplerCi.flags);
        util::AccumulateHash(hash, samplerCi.magFilter);
        util::AccumulateHash(hash, samplerCi.minFilter);
        util::AccumulateHash(hash, samplerCi.mipmapMode);
        util::AccumulateHash(hash, samplerCi.addressModeU);
        util::AccumulateHash(hash, samplerCi.addressModeV);
        util::AccumulateHash(hash, samplerCi.addressModeW);
        util::AccumulateHash(hash, samplerCi.mipLodBias);
        util::AccumulateHash(hash, samplerCi.anisotropyEnable);
        util::AccumulateHash(hash, samplerCi.maxAnisotropy);
        util::AccumulateHash(hash, samplerCi.compareEnable);
        util::AccumulateHash(hash, samplerCi.compareOp);
        util::AccumulateHash(hash, samplerCi.minLod);
        util::AccumulateHash(hash, samplerCi.maxLod);
        util::AccumulateHash(hash, samplerCi.borderColor);
        util::AccumulateHash(hash, samplerCi.unnormalizedCoordinates);
        return hash;
    }
}  // namespace foray::core