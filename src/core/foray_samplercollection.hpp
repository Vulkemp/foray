#pragma once
#include "foray_managedimage.hpp"
#include "foray_managedresource.hpp"
#include <unordered_map>

namespace foray::core {

    class SamplerCollection;

    /// @brief Wraps an image + sampler combination
    /// @remark Only useful if .pNext field of sampler createinfo remains zero. Image fields are optional.
    class CombinedImageSampler
    {
        friend SamplerCollection;

      public:
        CombinedImageSampler() = default;
        CombinedImageSampler(const CombinedImageSampler& other);
        CombinedImageSampler(CombinedImageSampler&& other);
        CombinedImageSampler& operator=(const CombinedImageSampler& other);
        virtual ~CombinedImageSampler();

        /// @brief Construct and initialize
        /// @param context Requires SamplerCol field
        /// @param image Image
        /// @param samplerCi Sampler create info (Note: DO NOT USE for non zero .pNext)
        CombinedImageSampler(core::Context* context, core::ManagedImage* image, const VkSamplerCreateInfo& samplerCi);

        /// @brief Initializes by fetching a matching sampler from sampler collection
        /// @param context Requires SamplerCol field
        /// @param samplerCi Sampler create info (Note: DO NOT USE for non zero .pNext)
        void Init(core::Context* context, const VkSamplerCreateInfo& samplerCi);
        /// @brief Initializes by fetching a matching sampler from sampler collection
        /// @param context Requires SamplerCol field
        /// @param image Image
        /// @param samplerCi Sampler create info (Note: DO NOT USE for non zero .pNext)
        void Init(core::Context* context, core::ManagedImage* image, const VkSamplerCreateInfo& samplerCi);

        /// @brief Returns the sampler and deinitializes
        void Destroy();

        /// @brief Build descriptor image info
        VkDescriptorImageInfo GetVkDescriptorInfo(VkImageLayout layout = VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) const
        {
            return VkDescriptorImageInfo{.sampler = mSampler, .imageView = (!!mManagedImage) ? mManagedImage->GetImageView() : nullptr, .imageLayout = layout};
        }

        FORAY_PROPERTY_ALL(ManagedImage)
        FORAY_PROPERTY_CGET(Sampler)
        FORAY_PROPERTY_CGET(Hash)
        FORAY_PROPERTY_CGET(Collection)

      protected:
        /// @brief Reference used only for descriptor image info filling
        core::ManagedImage* mManagedImage = nullptr;
        /// @brief Sampler (owned by SamplerCollection, this is a loan)
        VkSampler mSampler = nullptr;
        /// @brief Hash of the Sampler CreateInfo
        uint64_t mHash = (uint64_t)0;
        /// @brief Collection owning the Sampler object
        SamplerCollection* mCollection = nullptr;
    };

    /// @brief Provides sampler objects based on VkSamplerCreateInfo specifications.
    /// @remark Exclusively designed to work with CombinedImageSampler to ensure proper reference counting! Works only for VkSamplerCreateInfo without .pNext set
    /// Will count references to a unique sampler type, and automatically reuse sampler types and destroy
    class SamplerCollection : public core::VulkanResource<VkObjectType::VK_OBJECT_TYPE_SAMPLER>
    {
        friend CombinedImageSampler;

      public:
        SamplerCollection() = default;

        /// @brief Prepares for use
        /// @param context Requires DispatchTable
        inline void Init(core::Context* context) { mContext = context; }

        inline virtual bool Exists() const override { return mSamplerInstances.size() > 0; }

        /// @brief Make sure no reference to any existing sampler will be used after calling this!
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
