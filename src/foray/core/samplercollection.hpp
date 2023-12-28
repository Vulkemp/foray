#pragma once
#include "imageview.hpp"
#include "managedresource.hpp"
#include <unordered_map>

namespace foray::core {

    class SamplerCollection;

    /// @brief Represents a reference to a vk::Sampler object managed by a SamplerCollection
    /// @remark Only useful if .pNext field of sampler createinfo remains zero.
    class SamplerReference
    {
        friend SamplerCollection;

      public:
        SamplerReference() = default;
        SamplerReference(const SamplerReference& other);
        SamplerReference(SamplerReference&& other);
        SamplerReference& operator=(const SamplerReference& other);
        virtual ~SamplerReference();

        /// @brief Construct and initialize
        /// @param context Requires SamplerCol field
        /// @param samplerCi Sampler create info (Note: DO NOT USE for non zero .pNext)
        SamplerReference(Context* context, const vk::SamplerCreateInfo& samplerCi);
        /// @brief Construct and initialize
        /// @param collection Collection to get a sampler from
        /// @param samplerCi Sampler create info (Note: DO NOT USE for non zero .pNext)
        SamplerReference(SamplerCollection* collection, const vk::SamplerCreateInfo& samplerCi);

        /// @brief Initializes by fetching a matching sampler from sampler collection
        /// @param context Requires SamplerCol field
        /// @param samplerCi Sampler create info (Note: DO NOT USE for non zero .pNext)
        void Init(Context* context, const vk::SamplerCreateInfo& samplerCi);
        /// @brief Initializes by fetching a matching sampler from sampler collection
        /// @param collection Collection to get a sampler from
        /// @param samplerCi Sampler create info (Note: DO NOT USE for non zero .pNext)
        void Init(SamplerCollection* collection, const vk::SamplerCreateInfo& samplerCi);

        /// @brief Returns the sampler and deinitializes
        void Destroy();

        inline operator vk::Sampler() const { return mSampler; }

        FORAY_GETTER_V(Sampler)
        FORAY_GETTER_V(Hash)
        FORAY_GETTER_V(Collection)
      protected:
        /// @brief Sampler (owned by SamplerCollection, this is a loan)
        vk::Sampler mSampler;
        /// @brief Hash of the Sampler CreateInfo
        uint64_t mHash = (uint64_t)0;
        /// @brief Collection owning the Sampler object
        SamplerCollection* mCollection = nullptr;
    };

    /// @brief Wraps an image + sampler combination
    /// @remark Only useful if .pNext field of sampler createinfo remains zero. Image fields are optional.
    class CombinedImageSampler : public SamplerReference
    {
      public:
        CombinedImageSampler() = default;
        CombinedImageSampler(const CombinedImageSampler& other);
        CombinedImageSampler(CombinedImageSampler&& other);
        CombinedImageSampler& operator=(const CombinedImageSampler& other);

        /// @brief Construct and initialize
        /// @param context Requires SamplerCol field
        /// @param image Image
        /// @param samplerCi Sampler create info (Note: DO NOT USE for non zero .pNext)
        CombinedImageSampler(core::Context* context, core::ImageViewRef* image, const vk::SamplerCreateInfo& samplerCi);

        /// @brief Initializes by fetching a matching sampler from sampler collection
        /// @param context Requires SamplerCol field
        /// @param samplerCi Sampler create info (Note: DO NOT USE for non zero .pNext)
        void Init(core::Context* context, const vk::SamplerCreateInfo& samplerCi);
        /// @brief Initializes by fetching a matching sampler from sampler collection
        /// @param context Requires SamplerCol field
        /// @param image Image
        /// @param samplerCi Sampler create info (Note: DO NOT USE for non zero .pNext)
        void Init(core::Context* context, core::ImageViewRef* image, const vk::SamplerCreateInfo& samplerCi);

        /// @brief Build descriptor image info
        vk::DescriptorImageInfo GetVkDescriptorInfo(vk::ImageLayout layout = vk::ImageLayout::eReadOnlyOptimal) const
        {
            return vk::DescriptorImageInfo(mSampler, (!!mImageView) ? mImageView->GetView() : nullptr, layout);
        }

        FORAY_PROPERTY_V(ImageView)
      protected:
        /// @brief Reference used only for descriptor image info filling
        core::ImageViewRef* mImageView = nullptr;
    };

    /// @brief Provides sampler objects based on vk::SamplerCreateInfo specifications.
    /// @remark Exclusively designed to work with SamplerReference to ensure proper reference counting! Works only for vk::SamplerCreateInfo without .pNext set
    /// Will count references to a unique sampler type, and automatically reuse sampler types and destroy
    class SamplerCollection : public core::VulkanResource<vk::ObjectType::eSampler>
    {
        friend SamplerReference;

      public:

        /// @brief Prepares for use
        /// @param context Requires DispatchTable
        SamplerCollection(core::Context* context) : mContext(context) { }

        inline virtual bool Exists() const override { return mSamplerInstances.size() > 0; }

        virtual ~SamplerCollection() { ClearSamplerMap(); }

      protected:
        void GetSampler(SamplerReference& imageSampler, const vk::SamplerCreateInfo& samplerCi);

        void Register(SamplerReference& imageSampler);
        void Unregister(SamplerReference& imageSampler);

        void            ClearSamplerMap();
        static uint64_t GetHash(const vk::SamplerCreateInfo& samplerCi);

        core::Context* mContext = nullptr;

        struct SamplerInstance
        {
            vk::Sampler Sampler     = nullptr;
            uint64_t  SamplerHash = 0ULL;
            uint64_t  RefCount    = 0ULL;
        };

        std::unordered_map<uint64_t, SamplerInstance> mSamplerInstances;
    };
}  // namespace foray::core
