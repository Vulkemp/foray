#pragma once
#include "../memory/hsk_managedimage.hpp"
#include "../osi/hsk_window.hpp"
#include "hsk_framerenderinfo.hpp"
#include "hsk_minimalappbase.hpp"
#include "hsk_shadercompiler.hpp"
#include "hsk_vkcontext.hpp"
#include <vma/vk_mem_alloc.h>

namespace hsk {

    /// @brief Intended as base class for demo applications. Compared to MinimalAppBase it offers a complete simple vulkan setup.
    class DefaultAppBase : public MinimalAppBase
    {
      public:
        DefaultAppBase()          = default;
        virtual ~DefaultAppBase() = default;


        struct QueueInfo
        {
            VkQueue  Queue{};
            uint32_t QueueFamilyIndex{};
        };

        struct InFlightFrame
        {
            VkCommandBuffer CommandBuffer{};
            VkSemaphore     ImageAvailableSemaphore{};
            VkSemaphore     RenderFinishedSemaphore{};
            VkFence         CommandBufferExecutedFence{};
        };

        struct SwapchainImage
        {
            VkImage     Image;
            VkImageView ImageView;
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
        inline virtual void RecordCommandBuffer(FrameRenderInfo& renderInfo) {}
        virtual void        BaseSubmitFrame();

        struct DisplayConfig
        {
            hsk::Window                 Window       = {};
            VkSurfaceKHR                Surface      = {};
            vkb::Swapchain              SwapchainVkb = {};
            VkSwapchainKHR              Swapchain  = {};
            std::vector<SwapchainImage> SwapchainImages{};
        } mDisplayConfig;

        struct DeviceConfig
        {
            vkb::PhysicalDevice PhysicalDeviceVkb = {};
            VkPhysicalDevice    PhysicalDevice  = {};
            vkb::Device         DeviceVkb         = {};
            VkDevice            Device          = {};
        } mDeviceConfig;

        struct ShaderCompilerconfig
        {
            bool EnableShaderCompiler = true;
            /// @brief By default, shader source files are searched in the current working directory "cwd"/shaders.
            std::string ShaderSubdir = "/shaders/";
            /// @brief If mShaderSourceDirectoryPathFull is set to value, this path will be used as source dir.
            std::string ShaderSourceDirectoryPathFull = {};
            /// @brief If mShaderOutputDirectoryPathFull is set to value, this path will be used as output dir.
            std::string ShaderOutputDirectoryPathFull = {};
            /// @brief The shader compiler. See shader compiler options for further configuration.
            hsk::ShaderCompiler ShaderCompiler = {};
        } mShaderCompilerConfig;

        FrameRenderInfo mRenderInfo{};

#pragma region Vulkan
        /// @brief The applications vulkan context.
        VkContext mContext;


        std::vector<InFlightFrame> mFrames{};
        uint32_t                   mCurrentFrameIndex  = 0;
        uint64_t                   mRenderedFrameCount = 0;

        struct DeviceFeatures
        {
            VkPhysicalDeviceBufferDeviceAddressFeatures      bdafeatures;
            VkPhysicalDeviceRayTracingPipelineFeaturesKHR    rtpfeatures;
            VkPhysicalDeviceAccelerationStructureFeaturesKHR asfeatures;
            VkPhysicalDeviceDescriptorIndexingFeaturesEXT    difeatures;
        } mDeviceFeatures = {};

        struct SyncConfig
        {
            VkSemaphore ImageAvailableSemaphore{};
            VkSemaphore RenderFinishedSemaphore{};
            VkFence     CommandBufferExecutedFence{};
        } mSyncConfig;

        /// @brief Assuming the default queue supports graphics, transfer and compute. (TODO: are we sure, we don't need dedicated queues? For example dedicated transfer queues for asynchronous transfers)
        QueueInfo mDefaultQueue{};

        /// @brief Queue that supports presenting to the connected screen.
        QueueInfo mPresentQueue{};

        /// @brief Commandpool for the default queue.
        VkCommandPool mCommandPoolDefault{};

        VmaAllocator mAllocator{};

        /// @brief Points to the image thats to be copied into the swapchain image during rendering.
        ManagedImage* mSwapchainCopySourceImage{};

#pragma endregion
    };
}  // namespace hsk