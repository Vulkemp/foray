#pragma once
#include "hsk_exception.hpp"
#include <stdint.h>
#include <string>

namespace hsk {
    using fp32_t = float;
    using fp64_t = double;

    class NoMoveDefaults
    {
      public:
        NoMoveDefaults(const NoMoveDefaults& other)  = delete;
        NoMoveDefaults(const NoMoveDefaults&& other) = delete;
        NoMoveDefaults& operator=(const NoMoveDefaults& other) = delete;
    };
}  // namespace hsk