#include "foray_vulkanwindow.hpp"
#include "../foray_exception.hpp"
#include "../foray_logger.hpp"
#include "../foray_vulkan.hpp"
#include "foray_vulkandevice.hpp"
#include "foray_vulkaninstance.hpp"
#include <spdlog/fmt/fmt.h>

namespace foray::base {
    void VulkanWindow::CreateWindow()
    {
        if(!!mBeforeWindowCreateFunc)
        {
            mBeforeWindowCreateFunc(mWindow);
        }
        mWindow.Create();
    }
    void VulkanWindow::CreateSwapchain(vkb::Instance* instance, vkb::Device* device, vkb::DispatchTable* dispatchTable)
    {
        mInstance      = instance;
        mDevice        = device;
        mDispatchTable = dispatchTable;

        CreateSwapchain();
    }
    void VulkanWindow::CreateSwapchain()
    {
        Assert(mWindow.Exists(), "[VulkanWindow::CreateSwapchain] Unable to create swapchain without window initialized");
        Assert(!!mInstance, "[VulkanWindow::CreateSwapchain] Unable to create swapchain without mInstance set");
        Assert(!!mDevice, "[VulkanWindow::CreateSwapchain] Unable to create swapchain without mDevice set");
        Assert(!!mDispatchTable, "[VulkanWindow::CreateSwapchain] Unable to create swapchain without mDispatchTable set");

        if(!mSurface)
        {
            mSurface = mWindow.GetSurfaceKHR(*mInstance);
        }

        vkb::SwapchainBuilder swapchainBuilder(*mDevice, mSurface);

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
        FORAY_ASSERTFMT(ret.has_value(), "[VulkanWindow::CreateSwapchain] vkb Swapchain Builder failed to build swapchain. VkResult: {} Reason: {}", PrintVkResult(ret.vk_result()),
                        ret.error().message())

        mSwapchain = *ret;
    }
    void VulkanWindow::ExtractSwapchainImages()
    {
        uint32_t imageCount = mSwapchain.image_count;
        mSwapchainImages.resize(imageCount);

        // extract swapchain images
        auto images     = mSwapchain.get_images();
        auto imageviews = mSwapchain.get_image_views();
        Assert(images.has_value(), "[VulkanWindow::ExtractSwapchainImages] Failed to acquire swapchain images!");
        Assert(imageviews.has_value(), "[VulkanWindow::ExtractSwapchainImages] Failed to acquire swapchain image views!");
        for(uint32_t i = 0; i < imageCount; i++)
        {
            SwapchainImageInfo swapImage = mSwapchainImages[i];

            swapImage = SwapchainImageInfo{
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
                if(!!mDispatchTable)
                {
                    SetVulkanObjectName(mDispatchTable, VkObjectType::VK_OBJECT_TYPE_IMAGE, swapImage.Image, swapImage.Name);
                }
                else if(i == 0)
                {
                    logger()->warn("[VulkanWindow::ExtractSwapchainImages] mNameSwapchainImages enabled but mDispatchTable not set! Unable to set vulkan object names!");
                }
            }
        }
    }

    void VulkanWindow::RecreateSwapchain()
    {
        AssertVkResult(mDispatchTable->deviceWaitIdle());

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

    void VulkanWindow::DestroySwapchain()
    {
        for(auto& image : mSwapchainImages)
        {
            vkDestroyImageView(*mDevice, image.ImageView, nullptr);
        }
        mSwapchainImages.clear();
        if(!!mSwapchain.swapchain)
        {
            vkb::destroy_swapchain(mSwapchain);
            mSwapchain = vkb::Swapchain();
        }
    }

    void VulkanWindow::Destroy()
    {
        vkb::destroy_surface(*mInstance, mSurface);
        mSurface = nullptr;
        DestroySwapchain();
        mWindow.Destroy();
    }

}  // namespace foray::base
