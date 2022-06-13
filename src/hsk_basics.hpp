#pragma once
#include "hsk_exception.hpp"
#include "hsk_vkHelpers.hpp"
#include "base/hsk_vkcontext.hpp"
#include "base/hsk_logger.hpp"

#include <memory>
#include <stdint.h>
#include <string>

namespace hsk {
    using fp32_t = float;
    using fp64_t = double;

    class NoMoveDefaults
    {
      public:
        inline NoMoveDefaults()                      = default;
        NoMoveDefaults(const NoMoveDefaults& other)  = delete;
        NoMoveDefaults(NoMoveDefaults&& other) = default;
        NoMoveDefaults& operator=(const NoMoveDefaults& other) = delete;
    };

    class Polymorphic
    {
      protected:
        virtual void __makeMePolymorphic() {}
    };
}  // namespace hsk

#define HSK_PROPERTY_GET(member)                                                                                                                                                   \
    inline auto& Get##member() { return m##member; }
#define HSK_PROPERTY_CGET(member)                                                                                                                                                  \
    inline const auto& Get##member() const { return m##member; }
#define HSK_PROPERTY_SET(member)                                                                                                                                                   \
    template <typename TIn>                                                                                                                                                        \
    inline auto& Set##member(const TIn& value)                                                                                                                                     \
    {                                                                                                                                                                              \
        m##member = value;                                                                                                                                                         \
        return *this;                                                                                                                                                              \
    }

#define HSK_PROPERTY_ALL(member)                                                                                                                                                   \
    HSK_PROPERTY_GET(member)                                                                                                                                                       \
    HSK_PROPERTY_CGET(member)                                                                                                                                                      \
    HSK_PROPERTY_SET(member)
