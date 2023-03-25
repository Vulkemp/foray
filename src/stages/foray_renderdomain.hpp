#pragma once
#include "../foray_event.hpp"
#include "../foray_basics.hpp"
#include "../foray_vulkan.hpp"
#include "foray_stages_declares.hpp"
#include <unordered_set>

namespace foray::stages {

    /// @brief Maintains references to rendertargets operating in the same domain, forming a group operating on the same extent, and commonly the same rendertargets
    class RenderDomain
    {
      public:
        RenderDomain() = default;
        RenderDomain(std::string_view name, VkExtent2D extent) : mName(name), mExtent(extent) {}

        void InvokeResize(VkExtent2D extent);

        FORAY_PROPERTY_R(Name)
        FORAY_GETTER_V(Extent)

      protected:
        std::string mName   = "";
        VkExtent2D  mExtent = VkExtent2D{};

        FORAY_PRIORITYDELEGATE(VkExtent2D, Resized)
    };

}  // namespace foray::stages
