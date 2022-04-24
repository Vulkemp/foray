#pragma once
#include "hsk_exception.hpp"
#include <memory>
#include <stdint.h>
#include <string>

namespace hsk {
    using fp32_t = float;
    using fp64_t = double;

    class NoMoveDefaults
    {
      public:
        inline NoMoveDefaults() {}
        NoMoveDefaults(const NoMoveDefaults& other)  = delete;
        NoMoveDefaults(const NoMoveDefaults&& other) = delete;
        NoMoveDefaults& operator=(const NoMoveDefaults& other) = delete;
    };
}  // namespace hsk

#define HSK_PROPERTY_GET(member)                                                                                                                                                   \
    inline auto& Get##member() { return m##member; }
#define HSK_PROPERTY_CGET(member)                                                                                                                                                  \
    inline const auto& Get##member() const { return m##member; }
#define HSK_PROPERTY_SET(member)                                                                                                                                                   \
    inline auto& Set##member(const auto& value)                                                                                                                                         \
    {                                                                                                                                                                              \
        m##member = value;                                                                                                                                                         \
        return *this;                                                                                                                                                              \
    }

#define HSK_PROPERTY_ALL(member)                                                                                                                                                   \
    HSK_PROPERTY_GET(member)                                                                                                                                                       \
    HSK_PROPERTY_CGET(member)                                                                                                                                                      \
    HSK_PROPERTY_SET(member)
    