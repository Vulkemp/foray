#pragma once
#include <exception>
#include <spdlog/fmt/fmt.h>

namespace hsk {

    class Exception : std::exception
    {
      protected:
        std::string mReason = std::string("");

      public:
        inline Exception() : mReason("") {}
        inline explicit Exception(std::string_view reason) : mReason(reason) {}
        template <typename... Args>
        Exception(const char* const format, Args&&... args)
        {
            mReason = fmt::vformat(format, fmt::make_format_args(std::forward<Args>(args)...));
        }

        inline virtual const char* what() const noexcept override { return mReason.c_str(); }
    };
}  // namespace hsk