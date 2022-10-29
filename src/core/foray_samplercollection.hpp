#pragma once
#include "foray_managedimage.hpp"
#include "foray_managedresource.hpp"
#include <unordered_map>

namespace foray::core {

    class SamplerCollection;

    class CombinedImageSampler
    {
        friend SamplerCollection;

      public:
        CombinedImageSampler() = default;
        CombinedImageSampler(const CombinedImageSampler& other);
        CombinedImageSampler(CombinedImageSampler&& other);
        CombinedImageSampler& operator=(const CombinedImageSampler& other);
        virtual ~CombinedImageSampler();

        CombinedImageSampler(core::Context* context, core::ManagedImage* image, const VkSamplerCreateInfo& samplerCi);

        void Init(core::Context* context, const VkSamplerCreateInfo& samplerCi);
        void Init(core::Context* context, core::ManagedImage* image, const VkSamplerCreateInfo& samplerCi);

        void Destroy();

        VkDescriptorImageInfo GetVkDescriptorInfo(VkImageLayout layout = VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) const
        {
            return VkDescriptorImageInfo{.sampler = mSampler, .imageView = mManagedImage->GetImageView(), .imageLayout = layout};
        }

        FORAY_PROPERTY_ALL(ManagedImage)
        FORAY_PROPERTY_CGET(Sampler)
        FORAY_PROPERTY_CGET(Hash)
        FORAY_PROPERTY_CGET(Collection)

      protected:
        core::ManagedImage* mManagedImage = nullptr;
        VkSampler           mSampler      = nullptr;
        uint64_t            mHash         = (uint64_t)0;
        SamplerCollection*  mCollection   = nullptr;
    };

    class SamplerCollection : public core::VulkanResource<VkObjectType::VK_OBJECT_TYPE_SAMPLER>
    {
        friend CombinedImageSampler;

      public:
        SamplerCollection() = default;

        inline void Init(core::Context* context) { mContext = context; }

        inline virtual bool Exists() const override { return mSamplerInstances.size() > 0; }

        inline virtual void Destroy() override { ClearSamplerMap(); }
        inline virtual ~SamplerCollection() { ClearSamplerMap(); }

      protected:
        void GetSampler(CombinedImageSampler& imageSampler, const VkSamplerCreateInfo& samplerCi);

        void Register(CombinedImageSampler& imageSampler);
        void Unregister(CombinedImageSampler& imageSampler);

        void            ClearSamplerMap();
        static uint64_t GetHash(const VkSamplerCreateInfo& samplerCi);

        core::Context* mContext = nullptr;

        struct SamplerInstance
        {
            VkSampler Sampler     = nullptr;
            uint64_t  SamplerHash = 0ULL;
            uint64_t  RefCount    = 0ULL;
        };

        std::unordered_map<uint64_t, SamplerInstance> mSamplerInstances;
    };
}  // namespace foray::core
