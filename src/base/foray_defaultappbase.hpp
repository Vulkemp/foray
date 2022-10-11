#pragma once
#include "../core/foray_vkcontext.hpp"
#include "../osi/foray_window.hpp"
#include "foray_framerenderinfo.hpp"
#include "foray_minimalappbase.hpp"
#include <vma/vk_mem_alloc.h>

namespace foray::base {

    /// @brief Intended as base class for demo applications. Compared to MinimalAppBase it offers a complete simple vulkan setup.
    class DefaultAppBase : public MinimalAppBase
    {
      public:
        DefaultAppBase()          = default;
        virtual ~DefaultAppBase() = default;

        struct InFlightFrame
        {
            std::vector<VkCommandBuffer> CommandBuffers;
            VkSemaphore                  ImageAvailableSemaphore{};
            std::vector<VkSemaphore>     RenderFinishedSemaphores{};
            VkFence                      CommandBufferExecutedFence{};
        };

      protected:
        /// @brief Alter physical device selection.
        inline virtual void BeforePhysicalDeviceSelection(vkb::PhysicalDeviceSelector& pds){};

        /// @brief Alter device selection.
        inline virtual void BeforeDeviceBuilding(vkb::DeviceBuilder& deviceBuilder){};

        /// @brief Before building the swapchain
        inline virtual void BeforeSwapchainBuilding(vkb::SwapchainBuilder& swapchainBuilder){};

        inline virtual void BeforeSyncObjectCreation(uint32_t& inFlightFrameCount, uint32_t& perInFlightFrameCommandBufferCount){};

        /// @brief Base init is heavily overriden by this class, because a complete simple vulkan setup is included.
        virtual void BaseInit() override;

        virtual void BaseInitSelectPhysicalDevice();
        virtual void BaseInitBuildDevice();
        virtual void BaseInitBuildSwapchain();
        virtual void BaseInitGetVkQueues();
        virtual void BaseInitCommandPool();
        virtual void BaseInitCreateVma();
        virtual void BaseInitSyncObjects();

        virtual void        RecreateSwapchain();
        inline virtual void OnResized(VkExtent2D size) {}

        virtual void BaseCleanupSwapchain();
        virtual void BaseCleanupVulkan() override;

        virtual bool CanRenderNextFrame() override;
        virtual void Render(float delta) override;
        virtual void Update(float delta) override;

        virtual void        BasePrepareFrame();
        inline virtual void RecordCommandBuffer(FrameRenderInfo& renderInfo) {}
        inline virtual void QueryResultsAvailable(uint64_t frameIndex) {}
        virtual void        BaseSubmitFrame();

        void SetWindowDisplayMode(foray::EDisplayMode displayMode);

        virtual void OnShadersRecompiled(){};

        FrameRenderInfo mRenderInfo{};

#pragma region Vulkan
        /// @brief The applications vulkan context.
        core::VkContext mContext;
        uint32_t        mRequiredVulkanApiVersion = VK_API_VERSION_1_2;


        uint32_t                   mInFlightFrameCount                 = 0;
        uint32_t                   mPerInFlightFrameCommandBufferCount = 0;
        std::vector<InFlightFrame> mFrames{};
        uint32_t                   mCurrentFrameIndex  = 0;
        uint64_t                   mRenderedFrameCount = 0;

        struct DeviceFeatures
        {
            VkPhysicalDeviceBufferDeviceAddressFeatures      BufferDeviceAdressFeatures;
            VkPhysicalDeviceRayTracingPipelineFeaturesKHR    RayTracingPipelineFeatures;
            VkPhysicalDeviceAccelerationStructureFeaturesKHR AccelerationStructureFeatures;
            VkPhysicalDeviceDescriptorIndexingFeaturesEXT    DescriptorIndexingFeatures;
        } mDeviceFeatures = {};

        /// @brief Commandpool for the default queue.
        VkCommandPool mCommandPoolDefault{};

#pragma endregion
    };
}  // namespace foray::base