#include "foray_samplercollection.hpp"
#include "../util/foray_hash.hpp"
#include "foray_context.hpp"

namespace foray::core {
    SamplerReference::SamplerReference(const SamplerReference& other) : mSampler(other.mSampler), mHash(other.mHash), mCollection(other.mCollection)
    {
        if(!!mSampler && !!mCollection)
        {
            mCollection->Register(*this);
        }
    }
    SamplerReference::SamplerReference(SamplerReference&& other) : mSampler(other.mSampler), mHash(other.mHash), mCollection(other.mCollection)
    {
        if(!!mSampler && !!mCollection)
        {
            mCollection->Register(*this);
        }
    }
    SamplerReference& SamplerReference::operator=(const SamplerReference& other)
    {
        Destroy();
        mSampler    = other.mSampler;
        mHash       = other.mHash;
        mCollection = other.mCollection;
        if(!!mSampler && !!mCollection)
        {
            mCollection->Register(*this);
        }
        return *this;
    }
    SamplerReference::~SamplerReference()
    {
        Destroy();
    }
    SamplerReference::SamplerReference(SamplerCollection* collection, const VkSamplerCreateInfo& samplerCi)
    {
        mCollection = collection;
        if(!!mCollection)
        {
            mCollection->GetSampler(*this, samplerCi);
        }
    }
    SamplerReference::SamplerReference(Context* context, const VkSamplerCreateInfo& samplerCi)
    {
        mCollection = context->SamplerCol;
        if(!!mCollection)
        {
            mCollection->GetSampler(*this, samplerCi);
        }
    }
    void SamplerReference::Init(SamplerCollection* collection, const VkSamplerCreateInfo& samplerCi)
    {
        Destroy();
        mCollection = collection;
        if(!!mCollection)
        {
            mCollection->GetSampler(*this, samplerCi);
        }
    }
    void SamplerReference::Init(Context* context, const VkSamplerCreateInfo& samplerCi)
    {
        Destroy();
        mCollection = context->SamplerCol;
        if(!!mCollection)
        {
            mCollection->GetSampler(*this, samplerCi);
        }
    }
    void SamplerReference::Destroy()
    {
        if(!!mSampler && !!mCollection)
        {
            mCollection->Unregister(*this);
            mSampler    = nullptr;
            mCollection = nullptr;
        }
    }
    CombinedImageSampler::CombinedImageSampler(const CombinedImageSampler& other) : SamplerReference(other), mManagedImage(other.mManagedImage) {}
    CombinedImageSampler::CombinedImageSampler(CombinedImageSampler&& other) : SamplerReference(other), mManagedImage(other.mManagedImage) {}
    CombinedImageSampler& CombinedImageSampler::operator=(const CombinedImageSampler& other)
    {
        SamplerReference::operator=(other);
        mManagedImage = other.mManagedImage;
        return *this;
    }
    CombinedImageSampler::CombinedImageSampler(core::Context* context, core::ManagedImage* image, const VkSamplerCreateInfo& samplerCi)
        : SamplerReference(context, samplerCi), mManagedImage(image)
    {
    }
    void CombinedImageSampler::Init(core::Context* context, const VkSamplerCreateInfo& samplerCi)
    {
        SamplerReference::Init(context, samplerCi);
    }
    void CombinedImageSampler::Init(core::Context* context, core::ManagedImage* image, const VkSamplerCreateInfo& samplerCi)
    {
        mManagedImage = image;
        SamplerReference::Init(context, samplerCi);
    }

    void SamplerCollection::GetSampler(SamplerReference& samplerRef, const VkSamplerCreateInfo& samplerCi)
    {
        uint64_t hash = GetHash(samplerCi);
        auto     iter = mSamplerInstances.find(hash);
        if(iter != mSamplerInstances.end())
        {
            SamplerInstance& instance = iter->second;
            instance.RefCount++;
            samplerRef.mHash    = hash;
            samplerRef.mSampler = instance.Sampler;
        }
        else
        {
            SamplerInstance instance{.SamplerHash = hash, .RefCount = 1};
            AssertVkResult(mContext->VkbDispatchTable->createSampler(&samplerCi, nullptr, &(instance.Sampler)));
            mSamplerInstances[hash] = instance;
            samplerRef.mHash      = hash;
            samplerRef.mSampler   = instance.Sampler;
        }
    }
    void SamplerCollection::Register(SamplerReference& samplerRef)
    {
        mSamplerInstances[samplerRef.mHash].RefCount++;
    }
    void SamplerCollection::Unregister(SamplerReference& samplerRef)
    {
        auto iter = mSamplerInstances.find(samplerRef.mHash);
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