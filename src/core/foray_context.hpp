#pragma once
#include "../foray_vkb.hpp"
#include "../foray_vma.hpp"
#include "../foray_vulkan.hpp"
#include "../osi/foray_osi_declares.hpp"
#include "foray_core_declares.hpp"
#include "foray_swapchainimageinfo.hpp"

namespace foray::core {

    /// @brief Non owning context object
    /// @remark For many purposes, using a single Context object for an entire project would work out fine.
    /// Where it is is not, being trivially copyable allows the programmer to maintain multiple context objects with ease.
    /// Individual fields can be shared between many context objects, and changed at will
    struct Context
    {
        /// @brief The OsManager can be used to access windows, events, input devices
        osi::OsManager* OsManager = nullptr;
        /// @brief A window
        osi::Window* Window = nullptr;
        /// @brief A Vkb Instance
        vkb::Instance* VkbInstance = nullptr;
        /// @brief A Vkb PhysicalDevice
        vkb::PhysicalDevice* VkbPhysicalDevice = nullptr;
        /// @brief A Vkb Device
        vkb::Device* VkbDevice = nullptr;
        /// @brief A Vkb DispatchTable
        vkb::DispatchTable* VkbDispatchTable = nullptr;
        /// @brief A Vkb Swapchain
        vkb::Swapchain* Swapchain = nullptr;
        /// @brief Swapchain image infos (VkImage, VkImageView, Name, ...)
        std::vector<SwapchainImageInfo> SwapchainImages = {};
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

        inline operator VkInstance() const { return VkbInstance->instance; }
        inline operator VkPhysicalDevice() const { return VkbPhysicalDevice->physical_device; }
        inline operator VkDevice() const { return VkbDevice->device; }
        inline operator VkQueue() const { return Queue; }

        inline VkInstance       Instance() const { return VkbInstance->instance; }
        inline VkPhysicalDevice PhysicalDevice() const { return VkbPhysicalDevice->physical_device; }
        inline VkDevice         Device() const { return VkbDevice->device; }

        inline VkExtent2D GetSwapchainSize() const { return Swapchain->extent; }
    };
}  // namespace foray::core