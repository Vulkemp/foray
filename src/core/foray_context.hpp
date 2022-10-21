#pragma once
#include "../foray_vkb.hpp"
#include "../foray_vma.hpp"
#include "../foray_vulkan.hpp"
#include "../osi/foray_osi_declares.hpp"
#include "foray_swapchainimageinfo.hpp"

namespace foray::core {

    /// @brief Non owning context object
    struct Context
    {
        osi::OsManager*                 OsManager         = nullptr;
        vkb::Instance*                  VkbInstance       = nullptr;
        vkb::PhysicalDevice*            VkbPhysicalDevice = nullptr;
        vkb::Device*                    VkbDevice         = nullptr;
        vkb::DispatchTable*             VkbDispatchTable  = nullptr;
        vkb::Swapchain*                 Swapchain         = nullptr;
        std::vector<SwapchainImageInfo> SwapchainImages   = {};
        osi::Window*                    Window            = nullptr;
        VmaAllocator                    Allocator         = nullptr;
        VkQueue                         Queue             = nullptr;
        uint32_t                        QueueFamilyIndex  = VK_QUEUE_FAMILY_IGNORED;
        VkCommandPool                   CommandPool       = nullptr;

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