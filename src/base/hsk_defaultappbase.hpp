#pragma once
#include "../osi/hsk_window.hpp"
#include "hsk_minimalappbase.hpp"
#include "hsk_shadercompiler.hpp"
#include <vma/vk_mem_alloc.h>

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

        struct InFlightFrameRenderInfo
        {
            VkCommandBuffer CommandBuffer{};
            VkSemaphore     Ready{};
            VkSemaphore     Finished{};
            VkFence         CommandBufferExecuted{};
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
        virtual void BaseInitCompileShaders();
        virtual void BaseInitSyncObjects();

        virtual void        RecreateSwapchain();
        inline virtual void OnResized(VkExtent2D size) {}

        virtual void BaseCleanupSwapchain();
        virtual void BaseCleanupVulkan() override;

        virtual void Render(float delta) override;

        virtual void        BasePrepareFrame();
        inline virtual void RecordCommandBuffer(VkCommandBuffer cmdBuffer) {}
        virtual void        BaseSubmitFrame();

        /// @brief The main window used for rendering.
        hsk::Window mWindow;

        /// @brief If true, the app will try to automatically compile any shaders source files into spirv.
        bool mCompileShaders = true;
        /// @brief By default, shader source files are searched in the current working directory "cwd"/shaders.
        std::string mShaderSubdir{"/shaders/"};
        /// @brief If mShaderSourceDirectoryPathFull is set to value, this path will be used as source dir.
        std::string mShaderSourceDirectoryPathFull;
        /// @brief If mShaderOutputDirectoryPathFull is set to value, this path will be used as output dir.
        std::string mShaderOutputDirectoryPathFull;
        /// @brief The shader compiler. See shader compiler options for further configuration.
        ShaderCompiler mShaderCompiler;

#pragma region Vulkan
        VkSurfaceKHR mSurface{};

        vkb::PhysicalDevice mPhysicalDeviceVkb{};
        VkPhysicalDevice    mPhysicalDevice{};

        vkb::Device mDeviceVkb{};
        VkDevice    mDevice{};

        vkb::Swapchain mSwapchainVkb{};
        VkSwapchainKHR mSwapchain{};

        std::vector<InFlightFrameRenderInfo> mFrames{};
        uint32_t                             mCurrentFrameIndex  = 0;
        uint64_t                             mRenderedFrameCount = 0;

        struct DeviceFeatures
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