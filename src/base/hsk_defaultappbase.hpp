#pragma once
#include "../osi/hsk_window.hpp"
#include "hsk_minimalappbase.hpp"

namespace hsk {
    /// @brief Intended as base class for demo applications. Compared to MinimalAppBase it offers a complete simple vulkan setup.
    class DefaultAppBase : public MinimalAppBase
    {
      public:
        DefaultAppBase()          = default;
        virtual ~DefaultAppBase() = default;

        inline hsk::Window&         Window() { return mWindow; }
        inline VkSurfaceKHR         Surface() { return mSurface; }
        inline vkb::PhysicalDevice& VkbPhysicalDevice() { return mPhysicalDeviceVkb; }
        inline VkPhysicalDevice     PhysicalDevice() { return mPhysicalDevice; }
        inline vkb::Device&         VkbDevice() { return mDeviceVkb; }
        inline VkDevice             Device() { return mDevice; }
        inline vkb::Swapchain&      VkbSwapchain() { return mSwapchainVkb; }
        inline VkSwapchainKHR       Swapchain() { return mSwapchain; }

        struct QueueInfo
        {
            VkQueue  Queue{};
            uint32_t QueueFamilyIndex{};
        };

        inline QueueInfo& DefaultQueue() { return mDefaultQueue; }
        inline QueueInfo& PresentQueue() { return mPresentQueue; }

      protected:
        /// @brief Alter physical device selection.
        inline virtual void BeforePhysicalDeviceSelection(vkb::PhysicalDeviceSelector& pds){};

        /// @brief Alter device selection.
        inline virtual void BeforeDeviceBuilding(vkb::DeviceBuilder& deviceBuilder){};

        /// @brief Before building the swapchain
        inline virtual void BeforeSwapchainBuilding(vkb::SwapchainBuilder& swapchainBuilder){};

        /// @brief Base init is heavily overriden by this class, because a complete simple vulkan setup is included.
        virtual void BaseInit() override;

        virtual void BaseInitSelectPhysicalDevice();
        virtual void BaseInitBuildDevice();
        virtual void BaseInitBuildSwapchain();
        virtual void BaseInitGetVkQueues();
        virtual void BaseInitCommandPool();
        virtual void BaseInitCreateVma();

        virtual void BaseCleanupVulkan() override;

        /// @brief The main window used for rendering.
        hsk::Window mWindow;

#pragma region Vulkan
        VkSurfaceKHR mSurface{};

        vkb::PhysicalDevice mPhysicalDeviceVkb{};
        VkPhysicalDevice    mPhysicalDevice{};

        vkb::Device mDeviceVkb{};
        VkDevice    mDevice{};

        vkb::Swapchain mSwapchainVkb{};
        VkSwapchainKHR mSwapchain{};

        struct
        {
            VkPhysicalDeviceBufferDeviceAddressFeatures      bdafeatures;
            VkPhysicalDeviceRayTracingPipelineFeaturesKHR    rtpfeatures;
            VkPhysicalDeviceAccelerationStructureFeaturesKHR asfeatures;
            VkPhysicalDeviceDescriptorIndexingFeaturesEXT    difeatures;
        } mDeviceFeatures = {};

        /// @brief Assuming the default queue supports graphics, transfer and compute. (TODO: are we sure, we don't need dedicated queues? For example dedicated transfer queues for asynchronous transfers)
        QueueInfo mDefaultQueue{};

        /// @brief Queue that supports presenting to the connected screen.
        QueueInfo mPresentQueue{};

        /// @brief Commandpool for the default queue.
        VkCommandPool mCommandPoolDefault;

        VmaAllocator mAllocator;

#pragma endregion
    };
}  // namespace hsk