#include "rendertarget.hpp"

namespace foray::core {
    DefaultRenderTarget::DefaultRenderTarget(Context* context, std::string_view name, const Image::CreateInfo& imageCi) : mName(name), mImage(context, imageCi), mView(&mImage) {}

    DefaultRenderTarget::DefaultRenderTarget(Context* context, std::string_view name, const Image::CreateInfo& imageCi, const ImageSubResource& viewedSubResource)
        : mName(name), mImage(context, imageCi), mView(&mImage, viewedSubResource)
    {
    }

    DefaultRenderTarget::~DefaultRenderTarget() {}

    IImage* DefaultRenderTarget::GetImage()
    {
        return &mImage;
    }

    IImageView* DefaultRenderTarget::GetView()
    {
        return &mView;
    }

    std::string_view DefaultRenderTarget::GetName() const
    {
        return mName;
    }

    RenderTargetState::RenderTargetState(IRenderTarget* frameBuffer) : mFrameBuffer(frameBuffer) {}

    IRenderTarget* RenderTargetState::GetFrameBuffer()
    {
        return mFrameBuffer;
    }

    VkImageMemoryBarrier2 RenderTargetState::SetState(vk::PipelineStageFlags2 stage, vk::AccessFlags2 access, vk::ImageLayout layout)
    {
        VkImageMemoryBarrier2 barrier{.sType               = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                                      .srcStageMask        = mStage,
                                      .srcAccessMask       = mAccess,
                                      .dstStageMask        = stage,
                                      .dstAccessMask       = access,
                                      .oldLayout           = mLayout,
                                      .newLayout           = layout,
                                      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                                      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                                      .image               = mFrameBuffer->GetImage()->GetImage(),
                                      .subresourceRange    = mFrameBuffer->GetView()->GetSubResource().MakeVkSubresourceRange()};
        mStage  = stage;
        mAccess = access;
        mLayout = layout;
        return barrier;
    }

    std::string_view RenderTargetState::GetName() const
    {
        return mFrameBuffer->GetName();
    }

    ManualRenderTarget::ManualRenderTarget(std::string_view name, ImageViewRef* view) : mName(name), mView(view) {}

    ManualRenderTarget& ManualRenderTarget::SetView(ImageViewRef* view)
    {
        mView = view;
        return *this;
    }

    IImage* ManualRenderTarget::GetImage()
    {
        return mView->GetImage();
    }

    IImageView* ManualRenderTarget::GetView()
    {
        return mView;
    }

    std::string_view ManualRenderTarget::GetName() const
    {
        return mName;
    }

}  // namespace foray::core
