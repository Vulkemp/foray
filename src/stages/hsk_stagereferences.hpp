#pragma once
#include "../hsk_basics.hpp"
#include <vulkan/vulkan.h>

namespace hsk {

    struct StageImageInfo
    {
      public:
        enum class ESize
        {
            SwapchainExtent,
            Custom
        };

        ESize              Size        = ESize::SwapchainExtent;
        VkExtent3D         CustomSize  = {};
        VkFormat           Format      = VkFormat::VK_FORMAT_UNDEFINED;
        VkImageUsageFlags  UsageFlags  = 0;
        VkImageAspectFlags AspectFlags = 0;
        std::string        Name        = "";

        uint64_t GetRequirementsHash() const;
    };

    struct ResourceReferenceBase
    {
      public:
        virtual std::string_view GetName() = 0;
    };

    struct StageImageReference : public ResourceReferenceBase
    {
      public:
        enum class EReferenceType
        {
            Input,
            InputPreviousFrame,
            Output
        };

        EReferenceType ReferenceType = {};

        StageImageInfo ImageInfo;

        virtual std::string_view GetName() override { return ImageInfo.Name; }
    };

    struct CustomStageReference : public ResourceReferenceBase
    {
      public:
        std::string              Name;
        virtual std::string_view GetName() override { return Name; }
    };
}  // namespace hsk