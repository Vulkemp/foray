#pragma once
#include "../mem.hpp"
#include "../vulkan.hpp"
#include "../util/stringset.hpp"
#include "adapter.hpp"
#include <span>

namespace foray::base {
    class IVulkanInstance
    {
      public:
        virtual ~IVulkanInstance()             = default;
        virtual vk::Instance       Get()       = 0;
        virtual const vk::Instance Get() const = 0;
        inline virtual operator vk::Instance() { return Get(); }
        inline virtual operator const vk::Instance() const { return Get(); }

        virtual Ref<const util::StringSet> GetEnabledExtensions() const = 0;
        virtual Ref<const util::StringSet> GetEnabledLayers() const = 0;
        virtual std::span<const VulkanAdapterInfo> GetAdapterInfos() const = 0;
    };

    class IVulkanInstancePtr : public View<IVulkanInstance>
    {
      public:
        inline operator vk::Instance() { return GetRef().Get(); }
        inline operator const vk::Instance() const { return GetRef().Get(); }
    };

    class IVulkanDevice
    {
      public:
        virtual ~IVulkanDevice()                            = default;
        virtual vk::PhysicalDevice       GetAdapter()       = 0;
        virtual const vk::PhysicalDevice GetAdapter() const = 0;
        virtual vk::Device       Get()       = 0;
        virtual const vk::Device Get() const = 0;
        inline virtual operator vk::PhysicalDevice() { return GetAdapter(); }
        inline virtual operator const vk::PhysicalDevice() const { return GetAdapter(); }
        inline virtual operator vk::Device() { return Get(); }
        inline virtual operator const vk::Device() const { return Get(); }
    };

    class IVulkanDevicePtr : public View<IVulkanDevice>
    {
      public:
        inline virtual operator vk::PhysicalDevice() { return GetRef().GetAdapter(); }
        inline virtual operator const vk::PhysicalDevice() const { return GetRef().GetAdapter(); }
        inline operator vk::Device() { return GetRef().Get(); }
        inline operator const vk::Device() const { return GetRef().Get(); }
    };
}  // namespace foray::base
