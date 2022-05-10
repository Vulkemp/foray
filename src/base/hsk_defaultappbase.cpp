#include "hsk_defaultappbase.hpp"
#include "../hsk_env.hpp"
#include "../hsk_vkHelpers.hpp"
#include "../memory/hsk_intermediateImage.hpp"
#include "hsk_logger.hpp"
#include "vma/vk_mem_alloc.h"

namespace hsk {
    void DefaultAppBase::BaseInit()
    {
        logger()->info("Current working directory: {}", CurrentWorkingDirectory());
        // recompile shaders
        BaseInitCompileShaders();

        // create a window and add its requried instance extensions to the instance builder
        mWindow.Create();
        auto vulkanSurfaceExtensions = mWindow.GetVkSurfaceExtensions();
        for(const char* extension : vulkanSurfaceExtensions)
        {
            mVkbInstanceBuilder.enable_extension(extension);
        }

        mVkbInstanceBuilder.require_api_version(VK_API_VERSION_1_3);

        // create instance using instance builder from minimal app base
        MinimalAppBase::BaseInit();

        // get vulkan surface handle with created instance
        mSurface = mWindow.GetSurfaceKHR(mInstance);

        BaseInitSelectPhysicalDevice();
        BaseInitBuildDevice();
        BaseInitBuildSwapchain();
        BaseInitGetVkQueues();
        BaseInitCommandPool();
        BaseInitCreateVma();
        BaseInitSyncObjects();
    }

    void DefaultAppBase::BaseInitSelectPhysicalDevice()
    {
        // create physical device selector
        vkb::PhysicalDeviceSelector pds(mInstanceVkb, mSurface);

        {  // Configure device selector

            // Require capability to present to the current windows surface
            pds.require_present();
            pds.set_surface(mSurface);

            //// Basic minimums for raytracing
#ifndef DISABLE_RT_EXTENSIONS
            pds.set_minimum_version(1u, 1u);
            std::vector<const char*> requiredExtensions{VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,  // acceleration structure
                                                        VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,    // rt pipeline
                                                        // dependencies of acceleration structure
                                                        VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME, VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
                                                        VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
                                                        // dependencies of rt pipeline
                                                        VK_KHR_SPIRV_1_4_EXTENSION_NAME, VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME};
            pds.add_required_extensions(requiredExtensions);
#endif
        }

        // allow user to alter phyiscal device selection
        BeforePhysicalDeviceSelection(pds);

        // create phyiscal device
        auto physicalDeviceSelectionReturn = pds.select();
        if(!physicalDeviceSelectionReturn)
        {
            throw Exception("Physical device creation: {}", physicalDeviceSelectionReturn.error().message().c_str());
        }
        mPhysicalDeviceVkb = physicalDeviceSelectionReturn.value();
        mPhysicalDevice    = mPhysicalDeviceVkb.physical_device;
    }

    void DefaultAppBase::BaseInitBuildDevice()
    {
        // create logical device builder
        vkb::DeviceBuilder deviceBuilder{mPhysicalDeviceVkb};

        {  // Configure logical device builder

            // Adapted from https://github.com/KhronosGroup/Vulkan-Samples/blob/master/samples/extensions/raytracing_extended/raytracing_extended.cpp#L136
            // distributed via Apache 2.0 license https://github.com/KhronosGroup/Vulkan-Samples/blob/master/LICENSE

            mDeviceFeatures.bdafeatures.sType               = VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
            mDeviceFeatures.bdafeatures.bufferDeviceAddress = VK_TRUE;

#ifndef DISABLE_RT_EXTENSIONS

            deviceBuilder.add_pNext(&mDeviceFeatures.bdafeatures);

            mDeviceFeatures.rtpfeatures.sType              = VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
            mDeviceFeatures.rtpfeatures.rayTracingPipeline = VK_TRUE;
            deviceBuilder.add_pNext(&mDeviceFeatures.rtpfeatures);

            mDeviceFeatures.asfeatures.sType                 = VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
            mDeviceFeatures.asfeatures.accelerationStructure = VK_TRUE;
            deviceBuilder.add_pNext(&mDeviceFeatures.asfeatures);
#endif

            mDeviceFeatures.difeatures.sType                                     = VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT;
            mDeviceFeatures.difeatures.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
            deviceBuilder.add_pNext(&mDeviceFeatures.difeatures);

            // This currently causes a segfault, so commented out for the time being

            // auto &features = mVkbPhysicalDevice.request_extension_features<VkPhysicalDeviceDescriptorIndexingFeaturesEXT>(
            // 	VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT);
            // features.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;

            // if (mVkbPhysicalDevice.get_features().samplerAnisotropy)
            // {
            // 	mVkbPhysicalDevice.get_mutable_requested_features().samplerAnisotropy = true;
            // }
        }

        // allow user to alter device building
        BeforeDeviceBuilding(deviceBuilder);

        // automatically propagate needed data from instance & physical device
        auto deviceBuilderReturn = deviceBuilder.build();
        if(!deviceBuilderReturn)
        {
            throw Exception("Device creation: {}", deviceBuilderReturn.error().message());
        }
        mDeviceVkb = deviceBuilderReturn.value();
        mDevice    = mDeviceVkb.device;
        mVkContext.Device = mDeviceVkb.device;
    }

    void DefaultAppBase::BaseInitBuildSwapchain()
    {
        vkb::SwapchainBuilder swapchainBuilder(mDeviceVkb, mSurface);

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

        swapchainBuilder.add_format_feature_flags(VkFormatFeatureFlagBits::VK_FORMAT_FEATURE_TRANSFER_DST_BIT | VkFormatFeatureFlagBits::VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT);


        BeforeSwapchainBuilding(swapchainBuilder);

        auto swapchainBuilderReturn = swapchainBuilder.build();

        if(!swapchainBuilderReturn)
        {
            throw Exception("Swapchain building: {}", swapchainBuilderReturn.error().message());
        }

        mSwapchainVkb = swapchainBuilderReturn.value();
        mSwapchain    = mSwapchainVkb.swapchain;

        auto images     = mSwapchainVkb.get_images();
        auto imageviews = mSwapchainVkb.get_image_views();
        HSK_ASSERT(images.has_value(), "Failed to acquire swapchain images! Vkb Errorcode: {}")
        HSK_ASSERT(imageviews.has_value(), "Failed to acquire swapchain image views! Vkb Errorcode: {}")
        mSwapchainImages.resize(mSwapchainVkb.image_count);
        for(uint32_t i = 0; i < mSwapchainImages.size(); i++)
        {
            mSwapchainImages[i] = SwapchainImage{images.value()[i], imageviews.value()[i]};
        }
    }

    void DefaultAppBase::BaseInitGetVkQueues()
    {
        // Get the graphics queue with a helper function
        auto defaultQueueReturn = mDeviceVkb.get_queue(vkb::QueueType::graphics);
        if(!defaultQueueReturn)
        {
            throw Exception("Failed to get graphics queue. Error: {} ", defaultQueueReturn.error().message());
        }
        mDefaultQueue.Queue            = defaultQueueReturn.value();
        mDefaultQueue.QueueFamilyIndex = mDeviceVkb.get_queue_index(vkb::QueueType::graphics).value();

        auto presentQueueReturn = mDeviceVkb.get_queue(vkb::QueueType::present);
        if(!presentQueueReturn)
        {
            throw Exception("Failed to get graphics queue. Error: {} ", presentQueueReturn.error().message());
        }
        mPresentQueue.Queue            = presentQueueReturn.value();
        mPresentQueue.QueueFamilyIndex = mDeviceVkb.get_queue_index(vkb::QueueType::present).value();
    }

    void DefaultAppBase::BaseInitCommandPool()
    {
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        // VK_COMMAND_POOL_CREATE_TRANSIENT_BIT: Hint that command buffers are rerecorded with new commands very often (may change memory allocation behavior)
        // VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT: Allow command buffers to be rerecorded individually, without this flag they all have to be reset together
        poolInfo.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        poolInfo.queueFamilyIndex = mDefaultQueue.QueueFamilyIndex;

        VkResult result = vkCreateCommandPool(mDevice, &poolInfo, nullptr, &mCommandPoolDefault);
        if(result != VK_SUCCESS)
        {
            throw Exception("failed to create command pool! VkResult: {}", result);
        }
        mVkContext.CommandPool = mCommandPoolDefault;
    }

    void DefaultAppBase::BaseInitCreateVma()
    {
        VmaVulkanFunctions vulkanFunctions    = {};
        vulkanFunctions.vkGetInstanceProcAddr = &vkGetInstanceProcAddr;
        vulkanFunctions.vkGetDeviceProcAddr   = &vkGetDeviceProcAddr;

        VmaAllocatorCreateInfo allocatorCreateInfo = {};
        allocatorCreateInfo.vulkanApiVersion       = VK_API_VERSION_1_2;
        allocatorCreateInfo.physicalDevice         = mPhysicalDevice;
        allocatorCreateInfo.device                 = mDevice;
        allocatorCreateInfo.instance               = mInstance;
        allocatorCreateInfo.pVulkanFunctions       = &vulkanFunctions;

        vmaCreateAllocator(&allocatorCreateInfo, &mAllocator);
        mVkContext.Allocator = mAllocator;
    }

    void DefaultAppBase::BaseInitCompileShaders()
    {
        logger()->info("Compiling shaders...");
        std::string shaderSourceDirectory = CurrentWorkingDirectory() + mShaderSubdir;
        std::string shaderOutputDirectory = shaderSourceDirectory;
        if(mShaderSourceDirectoryPathFull.length() > 0)
        {

            shaderSourceDirectory = mShaderSourceDirectoryPathFull;
        }

        if(mShaderOutputDirectoryPathFull.length() > 0)
        {

            shaderOutputDirectory = mShaderOutputDirectoryPathFull;
        }

        mShaderCompiler.Init(shaderSourceDirectory, shaderOutputDirectory);
        mShaderCompiler.CompileAll();
        logger()->info("Compiling shaders successfully finished!");
    }

    void DefaultAppBase::BaseInitSyncObjects()
    {
        // https://www.asawicki.info/news_1647_vulkan_bits_and_pieces_synchronization_in_a_frame

        VkSemaphoreCreateInfo semaphoreCI{};
        semaphoreCI.sType = VkStructureType::VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceCI{};
        fenceCI.sType = VkStructureType::VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceCI.flags = VkFenceCreateFlagBits::VK_FENCE_CREATE_SIGNALED_BIT;

        // auto images     = mSwapchainVkb.get_images().value();
        // auto imageviews = mSwapchainVkb.get_image_views().value();
        for(uint32_t i = 0; i < mSwapchainVkb.image_count; i++)
        {
            InFlightFrame target{};
            // target.Image         = images[i];
            // target.ImageView     = imageviews[i];
            target.CommandBuffer = createCommandBuffer(mDevice, mCommandPoolDefault, VkCommandBufferLevel::VK_COMMAND_BUFFER_LEVEL_PRIMARY, false);

            HSK_ASSERT_VKRESULT(vkCreateSemaphore(mDevice, &semaphoreCI, nullptr, &target.ImageAvailableSemaphore));
            HSK_ASSERT_VKRESULT(vkCreateSemaphore(mDevice, &semaphoreCI, nullptr, &target.RenderFinishedSemaphore));

            HSK_ASSERT_VKRESULT(vkCreateFence(mDevice, &fenceCI, nullptr, &target.CommandBufferExecutedFence));

            mFrames.push_back(std::move(target));
        }
    }

    void DefaultAppBase::BaseCleanupVulkan()
    {
        HSK_ASSERT_VKRESULT(vkDeviceWaitIdle(mDevice));

        vkDestroyCommandPool(mDevice, mCommandPoolDefault, nullptr);
        for(auto& target : mFrames)
        {
            vkDestroySemaphore(mDevice, target.ImageAvailableSemaphore, nullptr);
            vkDestroySemaphore(mDevice, target.RenderFinishedSemaphore, nullptr);
            vkDestroyFence(mDevice, target.CommandBufferExecutedFence, nullptr);
        }

        BaseCleanupSwapchain();
        vkb::destroy_device(mDeviceVkb);
        mDeviceVkb = vkb::Device{};
        mDevice    = nullptr;
        vkb::destroy_surface(mInstanceVkb, mSurface);
        mSurface = nullptr;
        mWindow.Destroy();
        MinimalAppBase::BaseCleanupVulkan();
    }

    void DefaultAppBase::BaseCleanupSwapchain()
    {
        for(auto& image : mSwapchainImages)
        {
            vkDestroyImageView(mDevice, image.ImageView, nullptr);
            // vkDestroyImage(mDevice, image.Image, nullptr); Do not destroy images from swapchain
        }
        mSwapchainImages.resize(0);
        vkb::destroy_swapchain(mSwapchainVkb);
        mSwapchainVkb = vkb::Swapchain{};
        mSwapchain    = nullptr;
    }

    void DefaultAppBase::RecreateSwapchain()
    {
        HSK_ASSERT_VKRESULT(vkDeviceWaitIdle(mDevice));

        BaseCleanupSwapchain();

        if(!mWindow.Exists())
        {
            return;
        }

        BaseInitBuildSwapchain();

        OnResized(mSwapchainVkb.extent);
    }

    void DefaultAppBase::Render(float delta)
    {
        InFlightFrame& currentFrame = mFrames[mCurrentFrameIndex];

        // Make sure that the command buffer we want to use has been presented to the GPU
        vkWaitForFences(mDevice, 1, &currentFrame.CommandBufferExecutedFence, VK_TRUE, UINT64_MAX);

        VkImage primaryOutput    = nullptr;
        VkImage comparisonOutput = nullptr;

        // Record Render Command buffer

        FrameRenderInfo renderInfo(primaryOutput, comparisonOutput);
        renderInfo.SetFrameTime(delta).SetFrameNumber(mRenderedFrameCount).SetFrameObjectsIndex(mCurrentFrameIndex).SetCommandBuffer(currentFrame.CommandBuffer);

        HSK_ASSERT_VKRESULT(vkResetCommandBuffer(renderInfo.GetCommandBuffer(), 0));

        VkCommandBufferBeginInfo cmdbufBI{};
        cmdbufBI.sType = VkStructureType::VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        cmdbufBI.flags = VkCommandBufferUsageFlagBits::VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

        HSK_ASSERT_VKRESULT(vkBeginCommandBuffer(renderInfo.GetCommandBuffer(), &cmdbufBI));

        RecordCommandBuffer(renderInfo);


        // Get the next image index TODO: This action can be deferred until the command buffer section using the swapchain image is required. Should not be necessary however assuming sufficient in flight frames
        uint32_t swapChainImageIndex = 0;
        VkResult result              = vkAcquireNextImageKHR(mDevice, mSwapchain, UINT64_MAX, currentFrame.ImageAvailableSemaphore, nullptr, &swapChainImageIndex);

        if(result == VkResult::VK_ERROR_OUT_OF_DATE_KHR)
        {
            // Redo Swapchain
            RecreateSwapchain();
            return;
        }
        else if(result != VkResult::VK_SUBOPTIMAL_KHR)
        {
            AssertVkResult(result);
        }

        // Reset the fence
        HSK_ASSERT_VKRESULT(vkResetFences(mDevice, 1, &currentFrame.CommandBufferExecutedFence));

        // Record Command Buffer

        VkImageSubresourceRange range{};
        range.aspectMask     = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
        range.baseMipLevel   = 0;
        range.levelCount     = VK_REMAINING_MIP_LEVELS;
        range.baseArrayLayer = 0;
        range.layerCount     = VK_REMAINING_ARRAY_LAYERS;

        VkImageMemoryBarrier barrier{};
        barrier.sType            = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.subresourceRange = range;

        // Barrier: Grab swapchain image from present queue, change it into transfer layout
        barrier.srcAccessMask       = VkAccessFlagBits::VK_ACCESS_MEMORY_READ_BIT;
        barrier.dstAccessMask       = VkAccessFlagBits::VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.oldLayout           = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout           = VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.srcQueueFamilyIndex = mPresentQueue.QueueFamilyIndex;
        barrier.dstQueueFamilyIndex = mDefaultQueue.QueueFamilyIndex;
        barrier.image               = mSwapchainImages[swapChainImageIndex].Image;

        vkCmdPipelineBarrier(currentFrame.CommandBuffer, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TRANSFER_BIT, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0,
                             nullptr, 0, nullptr, 1, &barrier);

        // Clear swapchain image
        VkClearColorValue clearColor = VkClearColorValue{0.7f, 0.1f, 0.3f, 1.f};
        vkCmdClearColorImage(currentFrame.CommandBuffer, mSwapchainImages[swapChainImageIndex].Image, VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearColor, 1, &range);


        // Barrier : Convert GBuffer image layout into TRANSFER SOURCE optimal
        barrier.srcAccessMask       = VkAccessFlagBits::VK_ACCESS_MEMORY_READ_BIT;
        barrier.dstAccessMask       = VkAccessFlagBits::VK_ACCESS_TRANSFER_READ_BIT;
        barrier.oldLayout           = mImageCopySourceForRendering->GetImageLayout();
        barrier.newLayout           = VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcQueueFamilyIndex = mDefaultQueue.QueueFamilyIndex;
        barrier.dstQueueFamilyIndex = mDefaultQueue.QueueFamilyIndex;
        barrier.image               = mSwapchainImages[swapChainImageIndex].Image;

        vkCmdPipelineBarrier(currentFrame.CommandBuffer, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TRANSFER_BIT, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0,
                             nullptr, 0, nullptr, 1, &barrier);

        IntermediateImage::LayoutTransitionInfo info;
        info.CommandBuffer = currentFrame.CommandBuffer;
        info.BarrierSrcAccessMask = VkAccessFlagBits::VK_ACCESS_MEMORY_READ_BIT;
        info.BarrierDstAccessMask = VkAccessFlagBits::VK_ACCESS_TRANSFER_READ_BIT;
        info.NewImageLayout       = VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        mImageCopySourceForRendering->TransitionLayout()


        // Copy one of the g-buffer images into the swapchain
        VkImage       sourceImage  = mImageCopySourceForRendering->GetImage();
        VkImageLayout sourceLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
        VkImage       targetImage  = mSwapchainImages[swapChainImageIndex].Image;
        VkImageLayout targetLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        uint32_t      regionCount  = 1;
        VkImageCopy   region       = {};  // TODO: Are explicit values necessary??
        region.srcSubresource      = range;
        region.dstSubresource      = range;
        region.extent.width        = mSwapchainVkb.extent.width;
        region.extent.height       = mSwapchainVkb.extent.height;
        region.extent.depth        = 1;
        vkCmdCopyImage(currentFrame.CommandBuffer, sourceImage, sourceLayout, targetImage, targetLayout, regionCount, &region);

        // Barrier: Change swapchain image to present layout, transfer it back to present queue
        barrier.srcAccessMask       = VkAccessFlagBits::VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask       = VkAccessFlagBits::VK_ACCESS_MEMORY_READ_BIT;
        barrier.oldLayout           = VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout           = VkImageLayout::VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        barrier.srcQueueFamilyIndex = mDefaultQueue.QueueFamilyIndex;
        barrier.dstQueueFamilyIndex = mPresentQueue.QueueFamilyIndex;

        vkCmdPipelineBarrier(currentFrame.CommandBuffer, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TRANSFER_BIT, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0,
                             0, nullptr, 0, nullptr, 1, &barrier);


        HSK_ASSERT_VKRESULT(vkEndCommandBuffer(currentFrame.CommandBuffer));

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore          readySemaphores[]   = {currentFrame.ImageAvailableSemaphore};
        VkSemaphore          presentSemaphores[] = {currentFrame.RenderFinishedSemaphore};
        VkCommandBuffer      commandbuffers[]    = {currentFrame.CommandBuffer};
        VkPipelineStageFlags waitStages[]        = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT};
        submitInfo.waitSemaphoreCount            = 1;
        submitInfo.pWaitSemaphores               = readySemaphores;

        submitInfo.pWaitDstStageMask = waitStages;

        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores    = presentSemaphores;

        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers    = commandbuffers;

        // Submit all work to the default queue
        AssertVkResult(vkQueueSubmit(mDefaultQueue.Queue, 1, &submitInfo, currentFrame.CommandBufferExecutedFence));

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores    = presentSemaphores;

        VkSwapchainKHR swapChains[] = {mSwapchain};
        presentInfo.swapchainCount  = 1;
        presentInfo.pSwapchains     = swapChains;

        presentInfo.pImageIndices = &swapChainImageIndex;

        // Present on the present queue
        result = vkQueuePresentKHR(mPresentQueue.Queue, &presentInfo);

        if(result == VkResult::VK_ERROR_OUT_OF_DATE_KHR || result == VkResult::VK_SUBOPTIMAL_KHR)
        {
            // Redo Swapchain
            RecreateSwapchain();
        }
        else
        {
            AssertVkResult(result);
        }


        // Advance frame index
        mRenderedFrameCount++;
        mCurrentFrameIndex = (mCurrentFrameIndex + 1) % mFrames.size();
    }
    void DefaultAppBase::BasePrepareFrame() {}
    void DefaultAppBase::BaseSubmitFrame() {}
}  // namespace hsk