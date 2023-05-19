#pragma once
#include "../base/vulkandevice.hpp"
#include "../base/vulkaninstance.hpp"
#include "../base/vulkanwindowswapchain.hpp"
#include "../vma.hpp"
#include "../vulkan.hpp"
#include "../osi/osi_declares.hpp"
#include "core_declares.hpp"

namespace foray::core {

    /// @brief Non owning context object
    /// @remark For many purposes, using a single Context object for an entire project would work out fine.
    /// Where it is is not, being trivially copyable allows the programmer to maintain multiple context objects with ease.
    /// Individual fields can be shared between many context objects, and changed at will
    struct Context
    {
        /// @brief The OsManager can be used to access windows, events, input devices
        osi::OsManager* OsManager = nullptr;
        /// @brief Vulkan Instance
        base::VulkanInstance* Instance = nullptr;
        /// @brief Vulkan Device (with PhysicalDevice)
        base::VulkanDevice* Device = nullptr;
        /// @brief Vulkan Window with Swapchain
        base::VulkanWindowSwapchain* WindowSwapchain;
        /// @brief Vma Allocator
        VmaAllocator Allocator = nullptr;
        /// @brief Queue (in default setups used for all commands)
        VkQueue Queue = nullptr;
        /// @brief Queue Family Index
        uint32_t QueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        /// @brief Command Pool
        VkCommandPool CommandPool = nullptr;
        /// @brief Pipeline Cache
        VkPipelineCache PipelineCache = nullptr;
        /// @brief Sampler Collection
        SamplerCollection* SamplerCol = nullptr;
        /// @brief Shader Manager
        ShaderManager* ShaderMan = nullptr;

        inline operator VkInstance() { return *Instance; }
        inline operator VkPhysicalDevice() { return *Device; }
        inline operator VkDevice() { return *Device; }
        inline operator VkQueue() { return Queue; }

        inline VkInstance VkInstance() { return *Instance; }
        inline VkPhysicalDevice VkPhysicalDevice() { return *Device; }
        inline VkDevice VkDevice() { return *Device; }
        inline VkQueue VkQueue() { return Queue; }

        vkb::DispatchTable& DispatchTable() { return Device->GetDispatchTable(); }
    };
}  // namespace foray::core