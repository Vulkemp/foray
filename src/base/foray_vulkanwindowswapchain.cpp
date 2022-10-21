#include "foray_vulkanwindowswapchain.hpp"
#include "../core/foray_context.hpp"
#include "../foray_exception.hpp"
#include "../foray_logger.hpp"
#include "../foray_vulkan.hpp"
#include "../osi/foray_event.hpp"
#include "foray_vulkandevice.hpp"
#include "foray_vulkaninstance.hpp"
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
            mContext->Window = &mWindow;
        }
    }
    void VulkanWindowSwapchain::CreateSwapchain()
    {
        Assert(mWindow.Exists(), "[VulkanWindowSwapchain::CreateSwapchain] Unable to create swapchain without window initialized");
        Assert(!!mContext, "[VulkanWindowSwapchain::CreateSwapchain] Unable to create swapchain without mContext set");
        Assert(!!mContext->VkbInstance, "[VulkanWindowSwapchain::CreateSwapchain] Unable to create swapchain without Instance");
        Assert(!!mContext->VkbDevice, "[VulkanWindowSwapchain::CreateSwapchain] Unable to create swapchain without Device");
        Assert(!!mContext->VkbDispatchTable, "[VulkanWindowSwapchain::CreateSwapchain] Unable to create swapchain without DispatchTable");

        if(!mSurface)
        {
            mSurface = mWindow.GetOrCreateSurfaceKHR(mContext->Instance());
        }

        vkb::SwapchainBuilder swapchainBuilder(*(mContext->VkbDevice), mSurface);

        // default swapchain image formats:
        // color format: VK_FORMAT_B8G8R8A8_SRGB
        // color space: VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
        swapchainBuilder.use_default_format_selection();

        // tell vulkan the swapchain images will be used as color attachments
        swapchainBuilder.use_default_image_usage_flags();
        swapchainBuilder.add_image_usage_flags(VkImageUsageFlagBits::VK_IMAGE_USAGE_TRANSFER_DST_BIT);

        // use mailbox if possible, else fallback to fifo
        swapchainBuilder.use_default_present_mode_selection();
        swapchainBuilder.use_default_format_feature_flags();
        swapchainBuilder.add_format_feature_flags(VkFormatFeatureFlagBits::VK_FORMAT_FEATURE_BLIT_DST_BIT | VkFormatFeatureFlagBits::VK_FORMAT_FEATURE_TRANSFER_DST_BIT
                                                  | VkFormatFeatureFlagBits::VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT);

        if(!!mBeforeSwapchainBuildFunc)
        {
            mBeforeSwapchainBuildFunc(swapchainBuilder);
        }

        auto ret = swapchainBuilder.build();
        FORAY_ASSERTFMT(ret.has_value(), "[VulkanWindowSwapchain::CreateSwapchain] vkb Swapchain Builder failed to build swapchain. VkResult: {} Reason: {}",
                        PrintVkResult(ret.vk_result()), ret.error().message())

        mSwapchain = *ret;
        if(!!mContext)
        {
            mContext->Swapchain = &mSwapchain;
        }

        ExtractSwapchainImages();
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
            core::SwapchainImageInfo& swapImage = mSwapchainImages[i];

            swapImage = core::SwapchainImageInfo{
                .Image     = images.value()[i],
                .ImageView = imageviews.value()[i],
            };

            if(!!mMakeSwapchainImageNameFunc)
            {
                swapImage.Name = mMakeSwapchainImageNameFunc(i);
            }
            else
            {
                swapImage.Name = fmt::format("SwapImage #{}", i);
            }
            if(mNameSwapchainImages)
            {
                if(!!mContext)
                {
                    SetVulkanObjectName(mContext, VkObjectType::VK_OBJECT_TYPE_IMAGE, swapImage.Image, swapImage.Name);
                }
                else if(i == 0)
                {
                    logger()->warn("[VulkanWindowSwapchain::ExtractSwapchainImages] mNameSwapchainImages enabled but mContext not set! Unable to set vulkan object names!");
                }
            }
        }
        if(!!mContext)
        {
            mContext->SwapchainImages = mSwapchainImages;
        }
    }

    void VulkanWindowSwapchain::HandleEvent(const osi::Event* event)
    {
        const osi::EventWindowResized* resizeEvent = dynamic_cast<const osi::EventWindowResized*>(event);

        if(!!resizeEvent && resizeEvent->Source == &mWindow)
        {
            RecreateSwapchain();
        }
    }

    void VulkanWindowSwapchain::RecreateSwapchain()
    {
        AssertVkResult(mContext->VkbDispatchTable->deviceWaitIdle());

        if(mWindow.Exists())
        {
            DestroySwapchain();
        }
        else
        {
            Destroy();
            return;
        }

        CreateSwapchain();

        if(!!mOnResizedFunc)
        {
            mOnResizedFunc(mSwapchain.extent);
        }
    }

    void VulkanWindowSwapchain::DestroySwapchain()
    {
        if(!mContext)
        {
            return;
        }
        for(auto& image : mSwapchainImages)
        {
            mContext->VkbDispatchTable->destroyImageView(image.ImageView, nullptr);
        }
        mSwapchainImages.clear();
        mContext->SwapchainImages.clear();
        if(!!mSwapchain.swapchain)
        {
            vkb::destroy_swapchain(mSwapchain);
            mSwapchain = vkb::Swapchain();
        }
        mContext->Swapchain = nullptr;
    }

    void VulkanWindowSwapchain::Destroy()
    {
        DestroySwapchain();
        vkb::destroy_surface(mContext->Instance(), mSurface);
        mSurface = nullptr;
        mWindow.Destroy();
        mContext->Window = nullptr;
    }

}  // namespace foray::base
