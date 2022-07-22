#pragma once
#include "../hsk_basics.hpp"
#include <vulkan/vulkan.h>
#include <limits>

namespace hsk {
    template <VkFormat FORMAT>
    class ImageFormatTraits
    {
      public:
        inline static constexpr uint32_t COMPONENT_SIZE  = 0;
        inline static constexpr uint32_t COMPONENT_COUNT = 0;
        inline static constexpr uint32_t STRIDE          = COMPONENT_SIZE * COMPONENT_COUNT;
    };

    template <typename COMPONENT_T, COMPONENT_T ALPHA_FALLBACK_>
    class ComponentTraits
    {
      public:
        using COMPONENT                                  = COMPONENT_T;
        inline static constexpr uint32_t  SIZE           = sizeof(COMPONENT);
        inline static constexpr COMPONENT ALPHA_FALLBACK = (COMPONENT)ALPHA_FALLBACK_;
    };

    using ComponentTraits_Fp16 = ComponentTraits<uint16_t, 0x3C00>;
    using ComponentTraits_Fp32 = ComponentTraits<uint32_t, 0x3f800000>;  // Also an integer because floats can not be passed as template parameters
    using ComponentTraits_UInt32 = ComponentTraits<uint32_t, std::numeric_limits<uint32_t>::max()>;  // Also an integer because floats can not be passed as template parameters

    template <typename COMPONENT_TRAITS_, uint32_t COMPONENT_COUNT_>
    class ImageFormatTraitsBase;

    template <typename COMPONENT_TRAITS_>
    class ImageFormatTraitsBase<COMPONENT_TRAITS_, 4>
    {
      public:
        using COMPONENT_TRAITS                       = COMPONENT_TRAITS_;
        using COMPONENT                              = COMPONENT_TRAITS_::COMPONENT;
        inline static const uint32_t COMPONENT_COUNT = 4;
        inline static const uint32_t STRIDE          = COMPONENT_TRAITS::SIZE * 4;

        inline static void WriteColor(void* out, COMPONENT r, COMPONENT g, COMPONENT b, COMPONENT a)
        {
            COMPONENT* data = reinterpret_cast<COMPONENT*>(out);
            data[0]         = r;
            data[1]         = g;
            data[2]         = b;
            data[3]         = a;
        }

        inline static void WriteGrayscale(void* out, COMPONENT y, COMPONENT a)
        {
            COMPONENT* data = reinterpret_cast<COMPONENT*>(out);
            data[0]         = y;
            data[1]         = y;
            data[2]         = y;
            data[3]         = a;
        }
    };

    template <typename COMPONENT_TRAITS_>
    class ImageFormatTraitsBase<COMPONENT_TRAITS_, 3>
    {
      public:
        using COMPONENT_TRAITS                           = COMPONENT_TRAITS_;
        using COMPONENT                                  = COMPONENT_TRAITS_::COMPONENT;
        inline static constexpr uint32_t COMPONENT_COUNT = 3;
        inline static constexpr uint32_t STRIDE          = COMPONENT_TRAITS::SIZE * 3;

        inline static void WriteColor(void* out, COMPONENT r, COMPONENT g, COMPONENT b, COMPONENT a)
        {
            COMPONENT* data = reinterpret_cast<COMPONENT*>(out);
            data[0]         = r;
            data[1]         = g;
            data[2]         = b;
        }

        inline static void WriteGrayscale(void* out, COMPONENT y, COMPONENT a)
        {
            COMPONENT* data = reinterpret_cast<COMPONENT*>(out);
            data[0]         = y;
            data[1]         = y;
            data[2]         = y;
        }
    };

    template <typename COMPONENT_TRAITS_>
    class ImageFormatTraitsBase<COMPONENT_TRAITS_, 2>
    {
      public:
        using COMPONENT_TRAITS                           = COMPONENT_TRAITS_;
        using COMPONENT                                  = COMPONENT_TRAITS_::COMPONENT;
        inline static constexpr uint32_t COMPONENT_COUNT = 2;
        inline static constexpr uint32_t STRIDE          = COMPONENT_TRAITS::SIZE * 2;

        inline static void WriteColor(void* out, COMPONENT r, COMPONENT g, COMPONENT b, COMPONENT a)
        {
            COMPONENT* data = reinterpret_cast<COMPONENT*>(out);
            data[0]         = r;
            data[1]         = g;
        }

        inline static void WriteGrayscale(void* out, COMPONENT y, COMPONENT a)
        {
            COMPONENT* data = reinterpret_cast<COMPONENT*>(out);
            data[0]         = y;
            data[1]         = y;
        }
    };

    template <typename COMPONENT_TRAITS_>
    class ImageFormatTraitsBase<COMPONENT_TRAITS_, 1>
    {
      public:
        using COMPONENT_TRAITS                           = COMPONENT_TRAITS_;
        using COMPONENT                                  = COMPONENT_TRAITS_::COMPONENT;
        inline static constexpr uint32_t COMPONENT_COUNT = 1;
        inline static constexpr uint32_t STRIDE          = COMPONENT_TRAITS::SIZE * 1;

        inline static void WriteColor(void* out, COMPONENT r, COMPONENT g, COMPONENT b, COMPONENT a)
        {
            COMPONENT* data = reinterpret_cast<COMPONENT*>(out);
            data[0]         = r;
        }

        inline static void WriteGrayscale(void* out, COMPONENT y, COMPONENT a)
        {
            COMPONENT* data = reinterpret_cast<COMPONENT*>(out);
            data[0]         = y;
        }
    };


#pragma region float half
    template <>
    class ImageFormatTraits<VkFormat::VK_FORMAT_R16G16B16A16_SFLOAT> : public ImageFormatTraitsBase<ComponentTraits_Fp16, 4>
    {
    };
    template <>
    class ImageFormatTraits<VkFormat::VK_FORMAT_R16G16B16_SFLOAT> : public ImageFormatTraitsBase<ComponentTraits_Fp16, 3>
    {
    };
    template <>
    class ImageFormatTraits<VkFormat::VK_FORMAT_R16G16_SFLOAT> : public ImageFormatTraitsBase<ComponentTraits_Fp16, 2>
    {
    };
    template <>
    class ImageFormatTraits<VkFormat::VK_FORMAT_R16_SFLOAT> : public ImageFormatTraitsBase<ComponentTraits_Fp16, 1>
    {
    };
#pragma endregion
#pragma region float full
    template <>
    class ImageFormatTraits<VkFormat::VK_FORMAT_R32G32B32A32_SFLOAT> : public ImageFormatTraitsBase<ComponentTraits_Fp32, 4>
    {
    };
    template <>
    class ImageFormatTraits<VkFormat::VK_FORMAT_R32G32B32_SFLOAT> : public ImageFormatTraitsBase<ComponentTraits_Fp32, 3>
    {
    };
    template <>
    class ImageFormatTraits<VkFormat::VK_FORMAT_R32G32_SFLOAT> : public ImageFormatTraitsBase<ComponentTraits_Fp32, 2>
    {
    };
    template <>
    class ImageFormatTraits<VkFormat::VK_FORMAT_R32_SFLOAT> : public ImageFormatTraitsBase<ComponentTraits_Fp32, 1>
    {
    };
#pragma endregion
#pragma region inteer 32 unsigned
    template <>
    class ImageFormatTraits<VkFormat::VK_FORMAT_R32G32B32A32_UINT> : public ImageFormatTraitsBase<ComponentTraits_Fp32, 4>
    {
    };
    template <>
    class ImageFormatTraits<VkFormat::VK_FORMAT_R32G32B32_UINT> : public ImageFormatTraitsBase<ComponentTraits_Fp32, 3>
    {
    };
    template <>
    class ImageFormatTraits<VkFormat::VK_FORMAT_R32G32_UINT> : public ImageFormatTraitsBase<ComponentTraits_Fp32, 2>
    {
    };
    template <>
    class ImageFormatTraits<VkFormat::VK_FORMAT_R32_UINT> : public ImageFormatTraitsBase<ComponentTraits_Fp32, 1>
    {
    };
#pragma endregion

}  // namespace hsk