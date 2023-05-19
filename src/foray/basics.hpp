#pragma once
#include <memory>
#include <stdint.h>
#include <string>
#include <string_view>

#ifndef NDEBUG
#define FORAY_DEBUG 1
#endif

namespace foray {
    /// @brief stdint.h style 32 bit floating point type alias (float)
    using fp32_t = float;
    /// @brief stdint.h style 64 bit floating point type alias (double)
    using fp64_t = double;

    /// @brief Simple types for supressing automatic definition of duplicating move constructors & operator
    class NoMoveDefaults
    {
      public:
        inline NoMoveDefaults()                                = default;
        NoMoveDefaults(const NoMoveDefaults& other)            = delete;
        NoMoveDefaults(NoMoveDefaults&& other)                 = default;
        NoMoveDefaults& operator=(const NoMoveDefaults& other) = delete;
    };

    /// @brief Inflight frame count is the amount of frames 'in flight' aka frames recorded on the host before waiting for the device to finish rendering a previous frame.
    inline constexpr uint32_t INFLIGHT_FRAME_COUNT = 2;
}  // namespace foray

/// @brief Return value
#define FORAY_GETTER_V(member)                                                                                                                                                     \
    inline auto Get##member() const                                                                                                                                                \
    {                                                                                                                                                                              \
        return m##member;                                                                                                                                                          \
    }
/// @brief set by passing a value
#define FORAY_SETTER_V(member)                                                                                                                                                     \
    template <typename TIn>                                                                                                                                                        \
    inline auto& Set##member(TIn value)                                                                                                                                            \
    {                                                                                                                                                                              \
        m##member = value;                                                                                                                                                         \
        return *this;                                                                                                                                                              \
    }

/// @brief Return mutable reference
#define FORAY_GETTER_MR(member)                                                                                                                                                    \
    inline auto& Get##member()                                                                                                                                                     \
    {                                                                                                                                                                              \
        return m##member;                                                                                                                                                          \
    }
/// @brief Return constant reference
#define FORAY_GETTER_CR(member)                                                                                                                                                    \
    inline const auto& Get##member() const                                                                                                                                         \
    {                                                                                                                                                                              \
        return m##member;                                                                                                                                                          \
    }
/// @brief set by passing a constant reference
#define FORAY_SETTER_R(member)                                                                                                                                                     \
    template <typename TIn>                                                                                                                                                        \
    inline auto& Set##member(const TIn& value)                                                                                                                                     \
    {                                                                                                                                                                              \
        m##member = value;                                                                                                                                                         \
        return *this;                                                                                                                                                              \
    }

/// @brief Shorthand for mutable & constant reference getters
#define FORAY_GETTER_R(member)                                                                                                                                                     \
    FORAY_GETTER_MR(member)                                                                                                                                                        \
    FORAY_GETTER_CR(member)


/// @brief Getter+Setter shorthand for value types
#define FORAY_PROPERTY_V(member)                                                                                                                                                   \
    FORAY_GETTER_V(member)                                                                                                                                                         \
    FORAY_SETTER_V(member)

/// @brief Getter+Setter shorthand for reference types
#define FORAY_PROPERTY_R(member)                                                                                                                                                   \
    FORAY_GETTER_MR(member)                                                                                                                                                        \
    FORAY_GETTER_CR(member)                                                                                                                                                        \
    FORAY_SETTER_R(member)
