#pragma once
#include "../vulkan.hpp"
#include "core_declares.hpp"
#include "image.hpp"
#include "imageview.hpp"

namespace foray::core {
    /// @brief Represents an image which is mutable throughout the rendering process
    class IRenderTarget
    {
      public:
        virtual ~IRenderTarget() = default;
        /// @brief String Identifier of the contained resource
        virtual std::string_view GetName() const = 0;
        /// @brief Image
        virtual IImage* GetImage() = 0;
        /// @brief Image View. Must not represent more than one mip level and one array layer
        virtual IImageView* GetView() = 0;
    };

    class DefaultRenderTarget : public IRenderTarget, public NoMoveDefaults
    {
      public:
        DefaultRenderTarget(Context* context, std::string_view name, const Image::CreateInfo& imageCi);
        DefaultRenderTarget(Context* context, std::string_view name, const Image::CreateInfo& imageCi, const ImageSubResource& viewedSubResource);
        virtual ~DefaultRenderTarget();

        virtual std::string_view GetName() const override;
        virtual IImage*          GetImage() override;
        virtual IImageView*      GetView() override;

      private:
        std::string  mName;
        Image        mImage;
        ImageViewRef mView;
    };

    class ManualRenderTarget : public IRenderTarget
    {
      public:
        ManualRenderTarget(std::string_view name);
        ManualRenderTarget(std::string_view name, ImageViewRef* view);
        ManualRenderTarget& SetView(ImageViewRef* view);

        virtual std::string_view GetName() const override;
        virtual IImage*          GetImage() override;
        virtual IImageView*      GetView() override;

      private:
        std::string   mName;
        ImageViewRef* mView = nullptr;
    };

    /// @brief Tracks the state of a framebuffer resource over the duration of a single frame
    class RenderTargetState
    {
      public:
        RenderTargetState() = default;
        RenderTargetState(IRenderTarget* frameBuffer);

        IRenderTarget* GetFrameBuffer();
        /// @brief Updates the currently assumed state of the framebuffer
        /// @param stage The next pipeline stage to mask against
        /// @param access The next access mode to mask against
        /// @param layout The next image layout to transition to
        /// @return an appropriate image memory barrier representing a compatible layout transition
        VkImageMemoryBarrier2 SetState(vk::PipelineStageFlags2 stage, vk::AccessFlags2 access, vk::ImageLayout layout);

        std::string_view GetName() const;

        FORAY_GETTER_V(Stage)
        FORAY_GETTER_V(Access)
        FORAY_GETTER_V(Layout)

      private:
        IRenderTarget*        mFrameBuffer = nullptr;
        vk::PipelineStageFlags2 mStage       = vk::PipelineStageFlagBits2::eAllCommands;
        vk::AccessFlags2        mAccess      = vk::AccessFlagBits2::eMemoryRead | vk::AccessFlagBits2::eMemoryWrite;
        vk::ImageLayout         mLayout      = vk::ImageLayout::eUndefined;
    };
}  // namespace foray::core
