#pragma once
#include "../basics.hpp"
#include "../mem.hpp"
#include "core_declares.hpp"
#include "managedresource.hpp"

namespace foray::core {

    /// @brief Image interface, represents any form of vk::Image
    class IImage
    {
      public:
        /// @brief The underlying Vulkan image
        virtual vk::Image GetImage() const = 0;
        /// @brief Vulkan Format
        virtual vk::Format GetFormat() const = 0;
        /// @brief Size of the image
        virtual vk::Extent3D GetExtent() const = 0;
        /// @brief Total mip level count
        virtual uint32_t GetMipLevelCount() const = 0;
        /// @brief Total array layer count
        virtual uint32_t GetArrayLayerCount() const = 0;

        /// @brief Size of the image (1D or 2D type assumed)
        virtual VkExtent2D GetExtent2D() const;
    };

    /// @brief A general purpose implementation of the IImage interface
    class Image : public IImage, public VulkanResource<vk::ObjectType::eImage>
    {
        friend ImageViewRef;

      public:
        class CreateInfo
        {
          public:
            FORAY_PROPERTY_R(Name)
            FORAY_PROPERTY_V(PNext)
            FORAY_PROPERTY_FLAG(Flags)
            VkImageType GetType() const;
            FORAY_PROPERTY_V(Format)
            FORAY_PROPERTY_V(Extent)
            CreateInfo& SetExtent(uint32_t length);
            CreateInfo& SetExtent(uint32_t width, uint32_t height);
            CreateInfo& SetExtent(VkExtent2D extent);
            CreateInfo& SetExtent(uint32_t width, uint32_t height, uint32_t depth);
            VkExtent2D  GetExtent2D() const;
            uint32_t    GetExtent1D() const;
            FORAY_PROPERTY_V(MipLevelCount)
            FORAY_PROPERTY_V(ArrayLayerCount)
            FORAY_PROPERTY_V(SampleCount)
            FORAY_PROPERTY_V(Tiling)
            FORAY_PROPERTY_R(UsageFlags)
            FORAY_PROPERTY_R(QueueFamilyIndices)
            FORAY_PROPERTY_V(InitialLayout)

            FORAY_PROPERTY_FLAG(AllocationCreateFlags)
            FORAY_PROPERTY_V(MemoryUsage)
            FORAY_PROPERTY_R(RequiredMemoryPropertyFlags)
            FORAY_PROPERTY_R(PreferredMemoryPropertyFlags)
            FORAY_PROPERTY_V(MemoryTypeBits)
            FORAY_PROPERTY_V(MemoryPool)
            FORAY_PROPERTY_V(MemoryPriority)

            VkImageCreateInfo       GetVulkanCreateInfo() const;
            VmaAllocationCreateInfo GetVmaCreateInfo() const;

            void Validate() const;
            bool CheckFormatSupport(VkPhysicalDevice physicalDevice) const;
            void ValidateFormatSupport(VkPhysicalDevice physicalDevice) const;

            /// @brief Preconfigures an image create info for compute (storage) usage
            /// @remark Included ImageUsageFlags: Storage
            /// @param addBlitFlags Add TransferSrc and TransferDst ImageUsageFlagBits
            /// @param addSampledFlag Add Sampled ImageUsageFlagBit
            static CreateInfo PresetCompute(vk::Format format, VkExtent2D extent, bool addBlitFlags, bool addSampledFlag);
            /// @brief Preconfigures an image create info for compute (storage) usage
            /// @remark Included ImageUsageFlags: Storage
            /// @param addBlitFlags Add TransferSrc and TransferDst ImageUsageFlagBits
            /// @param addSampledFlag Add Sampled ImageUsageFlagBit
            static CreateInfo PresetCompute(vk::Format format, vk::Extent3D extent, bool addBlitFlags, bool addSampledFlag);
            /// @brief Preconfigures an image create info for texture usage
            /// @remark Included ImageUsageFlags: Sampled, TransferDst (TransferSrc if mipLevelCount > 1)
            static CreateInfo PresetTexture(vk::Format format, VkExtent2D extent, uint32_t mipLevelCount);
            /// @brief Preconfigures an image create info for texture usage
            /// @remark Included ImageUsageFlags: Sampled, TransferDst (TransferSrc if mipLevelCount > 1)
            static CreateInfo PresetTexture(vk::Format format, vk::Extent3D extent, uint32_t mipLevelCount);
            /// @brief Preconfigures an image create info for attachment (rasterizer output) usage
            /// @remark Included ImageUsageFlags: Attachment
            /// @param addBlitFlags Add TransferSrc and TransferDst ImageUsageFlagBits
            /// @param addSampledFlag Add Sampled ImageUsageFlagBit
            /// @param addStorageFlag Add Storage ImageUsageFlagBit
            static CreateInfo PresetAttachment(vk::Format format, VkExtent2D extent, bool addBlitFlags, bool addSampledFlag, bool addStorageFlag);
            /// @brief Preconfigures an image create info for attachment (rasterizer output) usage
            /// @remark Included ImageUsageFlags: Attachment
            /// @param addBlitFlags Add TransferSrc and TransferDst ImageUsageFlagBits
            /// @param addSampledFlag Add Sampled ImageUsageFlagBit
            /// @param addStorageFlag Add Storage ImageUsageFlagBit
            static CreateInfo PresetAttachment(vk::Format format, vk::Extent3D extent, bool addBlitFlags, bool addSampledFlag, bool addStorageFlag);

          protected:
            // Name
            std::string mName = "";

            // VkImageCreateInfo (inferred members: imageType from extent, sharingMode from queueFamilyIndices)
            void*                   mPNext              = nullptr;
            vk::ImageCreateFlags    mFlags;
            vk::Format              mFormat             = vk::Format::eUndefined;
            vk::Extent3D              mExtent             = vk::Extent3D{1u, 1u, 1u};
            uint32_t                mMipLevelCount      = 1u;
            uint32_t                mArrayLayerCount    = 1u;
            vk::SampleCountFlagBits mSampleCount        = vk::SampleCountFlagBits::e1;
            vk::ImageTiling         mTiling             = vk::ImageTiling::eOptimal;
            vk::ImageUsageFlags     mUsageFlags;
            std::vector<uint32_t>   mQueueFamilyIndices = {};
            vk::ImageLayout         mInitialLayout      = vk::ImageLayout::eUndefined;

            // VmaAllocationCreateInfo
            VmaAllocationCreateFlags mAllocationCreateFlags        = 0;
            VmaMemoryUsage           mMemoryUsage                  = VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
            VkMemoryPropertyFlags    mRequiredMemoryPropertyFlags  = 0;
            VkMemoryPropertyFlags    mPreferredMemoryPropertyFlags = 0;
            uint32_t                 mMemoryTypeBits               = ~0;
            VmaPool                  mMemoryPool                   = nullptr;
            fp32_t                   mMemoryPriority               = 1.f;
        };

        Image(core::Context* context, const CreateInfo& createInfo);

        virtual ~Image();

        FORAY_GETTER_V(Context)
        FORAY_GETTER_CR(CreateInfo)
        FORAY_GETTER_V(Allocation)
        FORAY_GETTER_CR(AllocationInfo)

        inline virtual vk::Image  GetImage() const override { return mImage; }
        inline virtual vk::Format GetFormat() const override { return mCreateInfo.GetFormat(); }
        inline virtual VkExtent2D GetExtent2D() const override { return mCreateInfo.GetExtent2D(); }
        inline virtual vk::Extent3D GetExtent() const override { return mCreateInfo.GetExtent(); }
        inline virtual uint32_t   GetMipLevelCount() const override { return mCreateInfo.GetMipLevelCount(); }
        inline virtual uint32_t   GetArrayLayerCount() const override { return mCreateInfo.GetArrayLayerCount(); }

      protected:
        core::Context*    mContext        = nullptr;
        CreateInfo        mCreateInfo     = CreateInfo();
        vk::Image         mImage          = nullptr;
        VmaAllocation     mAllocation     = {};
        VmaAllocationInfo mAllocationInfo = {};

        class RefCountedView
        {
          public:
            RefCountedView();
            RefCountedView(vk::ImageView view, ImageViewRef& initialRef);

            vk::ImageView              View = nullptr;
            std::vector<ImageViewRef*> Refs;
        };

        void GetImageView(ImageViewRef& view);
        void ReturnImageView(ImageViewRef& view);

        std::unordered_map<uint64_t, RefCountedView> mViews;
    };
}  // namespace foray::core

namespace foray {
    template <>
    class Local<core::Image> : public LocalBase<core::Image>
    {
      public:
        FORAY_LOCAL_SPECIALIZATION_DEFAULTS(core::Image)

        Local(core::Context* context, const core::Image::CreateInfo& createInfo);
        void New(core::Context* context, const core::Image::CreateInfo& createInfo);
        void Resize(VkExtent2D extent);
        void Resize(vk::Extent3D extent);
    };
}  // namespace foray
