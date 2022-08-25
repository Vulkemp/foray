#pragma once
#include "../hsk_basics.hpp"
#include <limits>
#include <vulkan/vulkan.h>

namespace hsk {

    /// @brief Describes the traits of a component type
    /// @param COMPONENT_T Machine typoe of the component
    /// @param ALPHA_FALLBACK Fallback for full opacity alpha if no value is provided
    template <typename COMPONENT_T, COMPONENT_T ALPHA_FALLBACK_, bool IS_FLOAT_, bool IS_SIGNED_>
    class ComponentTraits
    {
      public:
        /// @brief Component type
        using COMPONENT = COMPONENT_T;
        /// @brief Size (bytes) of the component
        inline static constexpr uint32_t SIZE = sizeof(COMPONENT);
        /// @brief Full opacity alpha fallback value
        inline static constexpr COMPONENT ALPHA_FALLBACK = (COMPONENT)ALPHA_FALLBACK_;
        /// @brief True if the internal representation is a floating point value
        inline static constexpr bool IS_FLOAT = IS_FLOAT_;
        /// @brief True if the internal representation supports negative values
        inline static constexpr bool IS_SIGNED = IS_SIGNED_;
    };

    using ComponentTraits_None = ComponentTraits<std::nullptr_t, nullptr, false, false>;

    /// @brief 16 bit float component type (1 component per channel)
    using ComponentTraits_Fp16   = ComponentTraits<uint16_t, 0x3C00, true, true>;      // Represented by an integer because x86 has no native support for half precision floats
    /// @brief 32 bit float component type (1 component per channel)
    using ComponentTraits_Fp32   = ComponentTraits<uint32_t, 0x3F800000, true, true>;  // Also an integer because floats can not be passed as template parameters
    /// @brief 64 bit float component type (1 component per channel)
    using ComponentTraits_Fp64   = ComponentTraits<uint64_t, 0x3FF0000000000000, true, true>;  // Also an integer because floats can not be passed as template parameters
    /// @brief 32 bit unsigned integer component type (1 component per channel)
    using ComponentTraits_UInt32 = ComponentTraits<uint32_t, std::numeric_limits<uint32_t>::max(), false, false>;
    /// @brief 8 bit unsigned integer component type (1 component per channel)
    using ComponentTraits_UInt8  = ComponentTraits<uint8_t, std::numeric_limits<uint8_t>::max(), false, false>;

    /// @brief 32 bit unsigned integer packed component type (2 bits alpha, 30 bits color, 1 component per texel, multiple channels per component)
    using ComponentTraits_PackedAlpha2Color30 =
        ComponentTraits<uint32_t, 0b11, false, false>;  // Component where the entire texel value is packed into a 32 bit value. Alpha has two bits

    /// @brief Describes the traits of a VkFormat value
    template <VkFormat FORMAT>
    class ImageFormatTraits
    {
      public:
        /// @brief Traits of the component
        using COMPONENT_TRAITS = ComponentTraits_None;
        /// @brief Count of components per texel (determines which channels it can represent)
        inline static constexpr uint32_t COMPONENT_COUNT = 0;
        /// @brief The stride in component base type (may differ from component count for packed types)
        inline static constexpr uint32_t COMPONENT_STRIDE = 4;
        /// @brief Stride (bytes) per texel
        inline static constexpr uint32_t BYTESTRIDE = COMPONENT_TRAITS::SIZE * COMPONENT_COUNT;
    };

    /// @brief Trait base class assembled from component trait type and component count
    template <typename COMPONENT_TRAITS_, uint32_t COMPONENT_COUNT_>
    class ImageFormatTraitsBase;

    template <typename COMPONENT_TRAITS_>
    class ImageFormatTraitsBase<COMPONENT_TRAITS_, 4>
    {
      public:
        using COMPONENT_TRAITS                            = COMPONENT_TRAITS_;
        using COMPONENT                                   = typename COMPONENT_TRAITS_::COMPONENT;
        inline static const uint32_t     COMPONENT_COUNT  = 4;
        inline static constexpr uint32_t COMPONENT_STRIDE = 4;
        inline static constexpr uint32_t BYTESTRIDE       = COMPONENT_TRAITS::SIZE * COMPONENT_STRIDE;

        /// @brief Write a texel in color
        inline static void WriteColor(void* out, COMPONENT r, COMPONENT g, COMPONENT b, COMPONENT a)
        {
            COMPONENT* data = reinterpret_cast<COMPONENT*>(out);
            data[0]         = r;
            data[1]         = g;
            data[2]         = b;
            data[3]         = a;
        }

        /// @brief Write a texel in grayscale
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
        using COMPONENT_TRAITS                            = COMPONENT_TRAITS_;
        using COMPONENT                                   = typename COMPONENT_TRAITS_::COMPONENT;
        inline static constexpr uint32_t COMPONENT_COUNT  = 3;
        inline static constexpr uint32_t COMPONENT_STRIDE = 3;
        inline static constexpr uint32_t BYTESTRIDE       = COMPONENT_TRAITS::SIZE * COMPONENT_STRIDE;

        /// @brief Write a texel in color
        inline static void WriteColor(void* out, COMPONENT r, COMPONENT g, COMPONENT b, COMPONENT a)
        {
            COMPONENT* data = reinterpret_cast<COMPONENT*>(out);
            data[0]         = r;
            data[1]         = g;
            data[2]         = b;
        }

        /// @brief Write a texel in grayscale
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
        using COMPONENT_TRAITS                            = COMPONENT_TRAITS_;
        using COMPONENT                                   = typename COMPONENT_TRAITS_::COMPONENT;
        inline static constexpr uint32_t COMPONENT_COUNT  = 2;
        inline static constexpr uint32_t COMPONENT_STRIDE = 2;
        inline static constexpr uint32_t BYTESTRIDE       = COMPONENT_TRAITS::SIZE * COMPONENT_STRIDE;

        /// @brief Write a texel in color
        inline static void WriteColor(void* out, COMPONENT r, COMPONENT g, COMPONENT b, COMPONENT a)
        {
            COMPONENT* data = reinterpret_cast<COMPONENT*>(out);
            data[0]         = r;
            data[1]         = g;
        }

        /// @brief Write a texel in grayscale
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
        using COMPONENT_TRAITS                            = COMPONENT_TRAITS_;
        using COMPONENT                                   = typename COMPONENT_TRAITS_::COMPONENT;
        inline static constexpr uint32_t COMPONENT_COUNT  = 1;
        inline static constexpr uint32_t COMPONENT_STRIDE = 1;
        inline static constexpr uint32_t BYTESTRIDE       = COMPONENT_TRAITS::SIZE * COMPONENT_STRIDE;

        /// @brief Write a texel in color
        inline static void WriteColor(void* out, COMPONENT r, COMPONENT g, COMPONENT b, COMPONENT a)
        {
            COMPONENT* data = reinterpret_cast<COMPONENT*>(out);
            data[0]         = r;
        }

        /// @brief Write a texel in grayscale
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
#pragma region float double
    template <>
    class ImageFormatTraits<VkFormat::VK_FORMAT_R64G64B64A64_SFLOAT> : public ImageFormatTraitsBase<ComponentTraits_Fp64, 4>
    {
    };
    template <>
    class ImageFormatTraits<VkFormat::VK_FORMAT_R64G64B64_SFLOAT> : public ImageFormatTraitsBase<ComponentTraits_Fp64, 3>
    {
    };
    template <>
    class ImageFormatTraits<VkFormat::VK_FORMAT_R64G64_SFLOAT> : public ImageFormatTraitsBase<ComponentTraits_Fp64, 2>
    {
    };
    template <>
    class ImageFormatTraits<VkFormat::VK_FORMAT_R64_SFLOAT> : public ImageFormatTraitsBase<ComponentTraits_Fp64, 1>
    {
    };
#pragma endregion
#pragma region integer 32 unsigned
    template <>
    class ImageFormatTraits<VkFormat::VK_FORMAT_R32G32B32A32_UINT> : public ImageFormatTraitsBase<ComponentTraits_UInt32, 4>
    {
    };
    template <>
    class ImageFormatTraits<VkFormat::VK_FORMAT_R32G32B32_UINT> : public ImageFormatTraitsBase<ComponentTraits_UInt32, 3>
    {
    };
    template <>
    class ImageFormatTraits<VkFormat::VK_FORMAT_R32G32_UINT> : public ImageFormatTraitsBase<ComponentTraits_UInt32, 2>
    {
    };
    template <>
    class ImageFormatTraits<VkFormat::VK_FORMAT_R32_UINT> : public ImageFormatTraitsBase<ComponentTraits_UInt32, 1>
    {
    };
#pragma endregion
#pragma region integer 8 unsigned
    template <>
    class ImageFormatTraits<VkFormat::VK_FORMAT_R8G8B8A8_UINT> : public ImageFormatTraitsBase<ComponentTraits_UInt8, 4>
    {
    };
    template <>
    class ImageFormatTraits<VkFormat::VK_FORMAT_R8G8B8_UINT> : public ImageFormatTraitsBase<ComponentTraits_UInt8, 3>
    {
    };
    template <>
    class ImageFormatTraits<VkFormat::VK_FORMAT_R8G8_UINT> : public ImageFormatTraitsBase<ComponentTraits_UInt8, 2>
    {
    };
    template <>
    class ImageFormatTraits<VkFormat::VK_FORMAT_R8_UINT> : public ImageFormatTraitsBase<ComponentTraits_UInt8, 1>
    {
    };
    template <>
    class ImageFormatTraits<VkFormat::VK_FORMAT_R8G8B8A8_UNORM> : public ImageFormatTraitsBase<ComponentTraits_UInt8, 4>
    {
    };
    template <>
    class ImageFormatTraits<VkFormat::VK_FORMAT_R8G8B8_UNORM> : public ImageFormatTraitsBase<ComponentTraits_UInt8, 3>
    {
    };
    template <>
    class ImageFormatTraits<VkFormat::VK_FORMAT_R8G8_UNORM> : public ImageFormatTraitsBase<ComponentTraits_UInt8, 2>
    {
    };
    template <>
    class ImageFormatTraits<VkFormat::VK_FORMAT_R8_UNORM> : public ImageFormatTraitsBase<ComponentTraits_UInt8, 1>
    {
    };
#pragma endregion
#pragma region integer packed 10 + 10 + 10 + 2 unsigned

    template <>
    class ImageFormatTraits<VkFormat::VK_FORMAT_A2R10G10B10_UINT_PACK32>
    {
      public:
        using COMPONENT_TRAITS                            = ComponentTraits_PackedAlpha2Color30;
        using COMPONENT                                   = COMPONENT_TRAITS::COMPONENT;
        inline static constexpr uint32_t COMPONENT_COUNT  = 4;
        inline static constexpr uint32_t COMPONENT_STRIDE = 1;
        inline static constexpr uint32_t BYTESTRIDE       = COMPONENT_TRAITS::SIZE * COMPONENT_STRIDE;

        /// @brief Write a texel in color
        inline static void WriteColor(void* out, COMPONENT r, COMPONENT g, COMPONENT b, COMPONENT a)
        {
            COMPONENT* data = reinterpret_cast<COMPONENT*>(out);
            data[0]         = ((a & 0b11) << 30) | ((r & 0b1111111111) << 20) | ((g & 0b1111111111) << 10) | (b & 0b1111111111);
        }

        /// @brief Write a texel in grayscale
        inline static void WriteGrayscale(void* out, COMPONENT y, COMPONENT a)
        {
            COMPONENT* data = reinterpret_cast<COMPONENT*>(out);
            y               = y & 0b1111111111;
            data[0]         = ((a & 0b11) << 30) | (y << 20) | (y << 20) | y;
        }
    };

}  // namespace hsk