#include "vulkanwindowswapchain.hpp"
#include "../core/context.hpp"
#include "../exception.hpp"
#include "../logger.hpp"
#include "../vulkan.hpp"
#include "../osi/osi_event.hpp"
#include "vulkandevice.hpp"
#include "vulkaninstance.hpp"
#include <spdlog/fmt/fmt.h>

namespace foray::base {
    void VulkanWindowSwapchain::CreateWindow()
    {
        if(!!mBeforeWindowCreateFunc)
        {
            mBeforeWindowCreateFunc(mWindow);
        }
        mWindow.Create();
        if(!!mContext)
        {
            mContext->WindowSwapchain = this;
        }
    }
    VkSurfaceKHR VulkanWindowSwapchain::GetOrCreateSurface()
    {
        if (!!mSurface)
        {
            return mSurface;
        }
        Assert(mWindow.Exists(), "[VulkanWindowSwapchain::GetOrCreateSurface] Window must be initialized");
        Assert(!!mContext, "[VulkanWindowSwapchain::GetOrCreateSurface] Require context set");
        Assert(!!mContext->Instance, "[VulkanWindowSwapchain::GetOrCreateSurface] Require Instance set");

        mSurface = mWindow.GetOrCreateSurfaceKHR(*mContext->Instance);
        return mSurface;
    }
    void VulkanWindowSwapchain::CreateSwapchain()
    {
        Assert(mWindow.Exists(), "[VulkanWindowSwapchain::CreateSwapchain] Unable to create swapchain without window initialized");
        Assert(!!mContext, "[VulkanWindowSwapchain::CreateSwapchain] Unable to create swapchain without mContext set");
        Assert(!!mContext->Instance, "[VulkanWindowSwapchain::CreateSwapchain] Unable to create swapchain without Instance");
        Assert(!!mContext->Device, "[VulkanWindowSwapchain::CreateSwapchain] Unable to create swapchain without Device");

        GetOrCreateSurface();

        vkb::SwapchainBuilder swapchainBuilder(mContext->Device->GetDevice(), mSurface);

        // default swapchain image formats:
        // color format: VK_FORMAT_B8G8R8A8_SRGB
        // color space: VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
        swapchainBuilder.use_default_format_selection();

        // tell vulkan the swapchain images will be used as color attachments and blit dest
        swapchainBuilder.add_image_usage_flags(vk::ImageUsageFlagBits::VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
        swapchainBuilder.add_image_usage_flags(vk::ImageUsageFlagBits::eTransferDst);

        // use mailbox if possible, else fallback to fifo
        swapchainBuilder.use_default_present_mode_selection();

        if(!!mBeforeSwapchainBuildFunc)
        {
            mBeforeSwapchainBuildFunc(swapchainBuilder);
        }

        auto ret = swapchainBuilder.build();
        FORAY_ASSERTFMT(ret.has_value(), "[VulkanWindowSwapchain::CreateSwapchain] vkb Swapchain Builder failed to build swapchain. vk::Result: {} Reason: {}",
                        PrintVkResult(ret.vk_result()), ret.error().message())

        mSwapchain = *ret;
        mExtent = mSwapchain.extent;

        ExtractSwapchainImages();

        InvokeResize(mExtent);
    }
    void VulkanWindowSwapchain::ExtractSwapchainImages()
    {
        uint32_t imageCount = mSwapchain.image_count;
        mSwapchainImages.resize(imageCount);

        // extract swapchain images
        auto images     = mSwapchain.get_images();
        auto imageviews = mSwapchain.get_image_views();
        Assert(images.has_value(), "[VulkanWindowSwapchain::ExtractSwapchainImages] Failed to acquire swapchain images!");
        Assert(imageviews.has_value(), "[VulkanWindowSwapchain::ExtractSwapchainImages] Failed to acquire swapchain image views!");
        for(uint32_t i = 0; i < imageCount; i++)
        {
            Local<core::SwapchainRenderTarget>& swapImage = mSwapchainImages[i];

            std::string_view name = "RT.Swapchain";

            swapImage.New(name, images.value()[i], imageviews.value()[i], mSwapchain.image_format, mSwapchain.extent);
        }
    }

    void VulkanWindowSwapchain::OnWindowResized(const osi::EventWindowResized* message)
    {
        if(message->Source == &mWindow && (message->Current.width != mSwapchain.extent.width || message->Current.height != mSwapchain.extent.height))
        {
            RecreateSwapchain();
        }
    }

    void VulkanWindowSwapchain::RecreateSwapchain()
    {
        AssertVkResult(mContext->DispatchTable().deviceWaitIdle());

        if(mWindow.Exists())
        {
            DestroySwapchain();
        }
        else
        {
            vkb::destroy_surface(mContext->vk::Instance(), mSurface);
            mSurface = nullptr;
            mWindow.Destroy();
            return;
        }

        CreateSwapchain();


        if(mSwapchain.extent.width != mExtent.width || mSwapchain.extent.height != mExtent.height)
        {
            mExtent = mSwapchain.extent;
            RenderDomain::InvokeResize(mExtent);
            if(!!mOnResizedFunc)
            {
                mOnResizedFunc(mSwapchain.extent);
            }
        }
    }

    core::SwapchainRenderTarget* VulkanWindowSwapchain::GetSwapchainFrameImage(uint32_t index)
    {
        Assert(index < mSwapchainImages.size(), "Index out of range.");
        return mSwapchainImages[index].Get();
    }

    const core::SwapchainRenderTarget* VulkanWindowSwapchain::GetSwapchainFrameImage(uint32_t index) const
    {
        Assert(index < mSwapchainImages.size(), "Index out of range.");
        return mSwapchainImages[index].Get();
    }

    uint32_t VulkanWindowSwapchain::GetSwapchainFrameImageCount() const
    {
        return (uint32_t)mSwapchainImages.size();
    }

    void VulkanWindowSwapchain::DestroySwapchain()
    {
        if(!mContext)
        {
            return;
        }
        for(auto& image : mSwapchainImages)
        {
            image->DestroyView(mContext);
        }
        mSwapchainImages.clear();
        if(!!mSwapchain.swapchain)
        {
            vkb::destroy_swapchain(mSwapchain);
            mSwapchain = vkb::Swapchain();
        }
        mExtent             = VkExtent2D{};
    }

    VulkanWindowSwapchain::~VulkanWindowSwapchain()
    {
        DestroySwapchain();
        vkb::destroy_surface(mContext->vk::Instance(), mSurface);
        mSurface = nullptr;
        mWindow.Destroy();
        mContext->WindowSwapchain = nullptr;
    }

}  // namespace foray::base
