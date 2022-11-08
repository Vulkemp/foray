#pragma once
#include <memory>
#include <stdint.h>
#include <string>
#include <string_view>

#ifndef NDEBUG
#define FORAY_DEBUG 1
#endif

namespace foray {
    using fp32_t = float;
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

    /// @brief Simple type for forcing a type to be polymorphic
    class Polymorphic
    {
      protected:
        virtual void __makeMePolymorphic() {}
    };

#if defined(FORAY_INFLIGHTFRAMECOUNT_OVERRIDE)
    /// @brief Inflight frame count is the amount of frames 'in flight' aka frames recorded on the host before waiting for the device to finish rendering a previous frame.
    inline constexpr uint32_t INFLIGHT_FRAME_COUNT = FORAY_INFLIGHTFRAMECOUNT_OVERRIDE;
#else
    /// @brief Inflight frame count is the amount of frames 'in flight' aka frames recorded on the host before waiting for the device to finish rendering a previous frame.
    inline constexpr uint32_t INFLIGHT_FRAME_COUNT = 2;
#endif
}  // namespace foray

#define FORAY_PROPERTY_GET(member)                                                                                                                                                   \
    inline auto& Get##member()                                                                                                                                                     \
    {                                                                                                                                                                              \
        return m##member;                                                                                                                                                          \
    }
#define FORAY_PROPERTY_CGET(member)                                                                                                                                                  \
    inline const auto& Get##member() const                                                                                                                                         \
    {                                                                                                                                                                              \
        return m##member;                                                                                                                                                          \
    }
#define FORAY_PROPERTY_SET(member)                                                                                                                                                   \
    template <typename TIn>                                                                                                                                                        \
    inline auto& Set##member(const TIn& value)                                                                                                                                     \
    {                                                                                                                                                                              \
        m##member = value;                                                                                                                                                         \
        return *this;                                                                                                                                                              \
    }

#define FORAY_PROPERTY_ALL(member)                                                                                                                                                   \
    FORAY_PROPERTY_GET(member)                                                                                                                                                       \
    FORAY_PROPERTY_CGET(member)                                                                                                                                                      \
    FORAY_PROPERTY_SET(member)

#define FORAY_PROPERTY_ALLGET(member)                                                                                                                                                \
    FORAY_PROPERTY_GET(member)                                                                                                                                                       \
    FORAY_PROPERTY_CGET(member)

#ifdef FORAY_ALTERNATIVE_PROPERTY_MACROS
/// @brief Return value
#define FORAY_GETTER_V(member)                                                                                                                                                     \
    inline auto Get##member() const                                                                                                                                                \
    {                                                                                                                                                                              \
        return m##member;                                                                                                                                                          \
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
/// @brief set by passing a value
#define FORAY_SETTER_V(member)                                                                                                                                                     \
    template <typename TIn>                                                                                                                                                        \
    inline auto& Set##member(TIn value)                                                                                                                                            \
    {                                                                                                                                                                              \
        m##member = value;                                                                                                                                                         \
        return *this;                                                                                                                                                              \
    }

/// @brief Getter+Setter shorthand for value types
#define FORAY_PROPERTY_V(member)\
    FORAY_GETTER_V(member)\
    FORAY_SETTER_V(member)

/// @brief Getter+Setter shorthand for reference types
#define FORAY_PROPERTY_R(member)\
    FORAY_GETTER_MR(member)\
    FORAY_GETTER_CR(member)\
    FORAY_SETTER_R(member)

#endif
