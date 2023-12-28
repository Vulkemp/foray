#pragma once
#include "../mem.hpp"
#include "../vulkan.hpp"
#include "core_declares.hpp"

namespace foray::core {
    class ImageSubResource
    {
      public:
        ImageSubResource() = default;
        ImageSubResource(Image* image);
        ImageSubResource(Image* image, vk::ImageViewType type, vk::ImageAspectFlags aspectFlags);
        ImageSubResource(vk::ImageViewType type, vk::ImageAspectFlags aspectFlags);
        ImageSubResource(vk::ImageViewType type, vk::ImageAspectFlags aspectFlags, Range mipLevels, Range arrayLayers);

        FORAY_PROPERTY_V(ViewType)
        FORAY_PROPERTY_R(AspectFlags)
        FORAY_PROPERTY_V(MipLevels)
        FORAY_PROPERTY_V(ArrayLayers)
        uint64_t CalculateHash() const;

        vk::ImageSubresourceRange MakeVkSubresourceRange() const;

      protected:
        vk::ImageViewType    mViewType    = vk::ImageViewType::e2D;
        vk::ImageAspectFlags mAspectFlags;
        Range                mMipLevels   = {0, 1};
        Range                mArrayLayers = {0, 1};
    };

    class IImageView
    {
      public:
        virtual vk::ImageView           GetView() const        = 0;
        virtual IImage*                 GetImage()             = 0;
        virtual const IImage*           GetImage() const       = 0;
        virtual const ImageSubResource& GetSubResource() const = 0;

        virtual vk::Format GetFormat() const;
        virtual vk::Extent3D GetExtent() const;
        virtual VkExtent2D GetExtent2D() const;
    };

    class ImageViewRef : public IImageView, public NoMoveDefaults
    {
        friend Image;

      public:
        ImageViewRef() = default;
        ImageViewRef(Image* image);
        ImageViewRef(Image* image, ImageSubResource viewResource);

        virtual ~ImageViewRef();

        void Destroy();
        bool Exists() const;

        virtual vk::ImageView           GetView() const override;
        virtual IImage*                 GetImage() override;
        virtual IImage*                 GetImage() const override;
        virtual const ImageSubResource& GetSubResource() const override;

      protected:
        vk::ImageView    mView        = nullptr;
        Image*           mImage       = nullptr;
        ImageSubResource mSubResource = ImageSubResource();
    };
}  // namespace foray::core

namespace foray {
    template <>
    class Local<core::ImageViewRef> : public LocalBase<core::ImageViewRef>
    {
      public:
        FORAY_LOCAL_SPECIALIZATION_DEFAULTS(core::ImageViewRef)

        inline Local(core::Image* image);
        inline Local(core::Image* image, core::ImageSubResource viewResource);
        inline void New(core::Image* image);
        inline void New(core::Image* image, core::ImageSubResource viewResource);
    };
    inline Local<core::ImageViewRef>::Local(core::Image* image) : LocalBase<core::ImageViewRef>(image) {}
    inline Local<core::ImageViewRef>::Local(core::Image* image, core::ImageSubResource viewResource) : LocalBase<core::ImageViewRef>(image, viewResource) {}
    inline void Local<core::ImageViewRef>::New(core::Image* image)
    {
        LocalBase<core::ImageViewRef>::New(image);
    }
    inline void Local<core::ImageViewRef>::New(core::Image* image, core::ImageSubResource viewResource)
    {
        LocalBase<core::ImageViewRef>::New(image, viewResource);
    }
}  // namespace foray
