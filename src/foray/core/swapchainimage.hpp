#pragma once
#include "../vulkan.hpp"
#include "rendertarget.hpp"

namespace foray::core {
    /// @brief Collects information for a swapchain image
    class SwapchainImage : public IImage
    {
      public:
        SwapchainImage() = default;
        SwapchainImage(std::string_view name, vk::Image image, vk::Format format, VkExtent2D extent);

        virtual vk::Image    GetImage() const override;
        virtual vk::Format   GetFormat() const override;
        virtual vk::Extent3D GetExtent() const override;
        virtual uint32_t   GetMipLevelCount() const override;
        virtual uint32_t   GetArrayLayerCount() const override;

      protected:
        /// @brief Debug name given
        std::string mName = "";
        /// @brief vk::Image
        vk::Image    mImage  = nullptr;
        vk::Format   mFormat = vk::Format::eUndefined;
        vk::Extent3D mExtent = {};
    };

    class SwapchainImageView : public IImageView
    {
      public:
        SwapchainImageView() = default;
        SwapchainImageView(SwapchainImage* image, vk::ImageView view);

        virtual vk::ImageView             GetView() const override;
        virtual IImage*                 GetImage() override;
        virtual const IImage*           GetImage() const override;
        virtual const ImageSubResource& GetSubResource() const override;

      protected:
        SwapchainImage*  mImage       = nullptr;
        vk::ImageView      mView        = nullptr;
        ImageSubResource mSubResource = ImageSubResource(vk::ImageViewType::e2D, vk::ImageAspectFlagBits::eColor);
    };

    class SwapchainRenderTarget : public IRenderTarget
    {
      public:
        SwapchainRenderTarget() = default;
        SwapchainRenderTarget(std::string_view name, vk::Image image, vk::ImageView view, vk::Format format, VkExtent2D extent);
        void DestroyView(Context* context);

        virtual std::string_view GetName() const override;
        virtual IImage*          GetImage() override;
        virtual IImageView*      GetView() override;

      protected:
        std::string        mName;
        SwapchainImage     mImage;
        SwapchainImageView mView;
    };
}  // namespace foray::core