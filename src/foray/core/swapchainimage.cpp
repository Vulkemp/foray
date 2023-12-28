#include "swapchainimage.hpp"
#include "context.hpp"

namespace foray::core {
    SwapchainImage::SwapchainImage(std::string_view name, vk::Image image, vk::Format format, VkExtent2D extent)
        : mName(name), mImage(image), mFormat(format), mExtent{extent.width, extent.height, 1u}
    {
    }
    vk::Image SwapchainImage::GetImage() const
    {
        return mImage;
    }
    vk::Format SwapchainImage::GetFormat() const
    {
        return mFormat;
    }
    vk::Extent3D SwapchainImage::GetExtent() const
    {
        return mExtent;
    }
    uint32_t SwapchainImage::GetMipLevelCount() const
    {
        return 1u;
    }
    uint32_t SwapchainImage::GetArrayLayerCount() const
    {
        return 1u;
    }
    SwapchainImageView::SwapchainImageView(SwapchainImage* image, vk::ImageView view) : mImage(image), mView(view) {}
    vk::ImageView SwapchainImageView::GetView() const
    {
        return mView;
    }
    IImage* SwapchainImageView::GetImage()
    {
        return mImage;
    }
    const IImage* SwapchainImageView::GetImage() const
    {
        return mImage;
    }
    const ImageSubResource& SwapchainImageView::GetSubResource() const
    {
        return mSubResource;
    }
    SwapchainRenderTarget::SwapchainRenderTarget(std::string_view name, vk::Image image, vk::ImageView view, vk::Format format, VkExtent2D extent)
        : mName(name), mImage(name, image, format, extent), mView(&mImage, view)
    {
    }
    void SwapchainRenderTarget::DestroyView(Context* context)
    {
        if(!!mView.GetView())
        {
            vkDestroyImageView(context->VkDevice(), mView.GetView(), nullptr);
            mView = SwapchainImageView();
        }
    }
    std::string_view SwapchainRenderTarget::GetName() const
    {
        return mName;
    }
    IImage* SwapchainRenderTarget::GetImage()
    {
        return &mImage;
    }
    IImageView* SwapchainRenderTarget::GetView()
    {
        return &mView;
    }
}  // namespace foray::core