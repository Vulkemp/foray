#pragma once
#include <exception>
#include <spdlog/fmt/fmt.h>

namespace hsk {

    class Exception : std::exception
    {
      public:
        inline Exception() : mReason("") {}
        inline explicit Exception(std::string_view reason) : mReason(reason) {}
        template <typename... Args>
        Exception(const char* const format, Args&&... args)
        {
            mReason = fmt::vformat(format, fmt::make_format_args(std::forward<Args>(args)...));
        }

        inline virtual const char* what() const noexcept override { return mReason.c_str(); }

        static void Throw();
        static void Throw(std::string_view reason);
        template <typename... Args>
        static void Throw(const char* const format, Args&&... args)
        {
            std::string reason = fmt::vformat(format, fmt::make_format_args(std::forward<Args>(args)...));
            Throw(reason);
        }

      protected:
        std::string mReason = std::string("");
    };

}  // namespace hsk

#define HSK_ASSERT(val, msg)                                                                                                                                                       \
    if(!(val))                                                                                                                                                                     \
    {                                                                                                                                                                              \
        hsk::Exception::Throw(msg);                                                                                                                                                \
    }

#define HSK_ASSERTV(val, msg, ...)                                                                                                                                                 \
    if(!(val))                                                                                                                                                                     \
    {                                                                                                                                                                              \
        hsk::Exception::Throw(msg, __VA_ARGS__);                                                                                                                                   \
    }
