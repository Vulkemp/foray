#pragma once
#include <exception>
#include <source_location>
#include <spdlog/fmt/fmt.h>

#ifndef __cpp_lib_source_location
#pragma message "Oh boy"
#endif

namespace hsk {

    /// @brief An extension of std::exception providing support for formatted error messages. Exceptions should always be catched via "catch (const std::exception& ex)", and thrown by value (Just use builtin static throw functions)
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

        // /// @brief Throws an exception (by value, catch via "const std::exception& ex") with a generic error message. Avoid use.
        // [[ noreturn ]] static void Throw();
        // /// @brief Throws an exception (by value, catch via "const std::exception& ex") with a user defined error message
        // [[ noreturn ]] static void Throw(std::string_view reason);
        // /// @brief Throws an exception (by value, catch via "const std::exception& ex") with a user defined error message
        // template <typename... Args>
        // [[ noreturn ]] inline static void Throw(const char* const format, Args&&... args)
        // {
        //     std::string reason = fmt::vformat(format, fmt::make_format_args(std::forward<Args>(args)...));
        //     Throw(reason);
        // }

        /// @brief Throws an exception (by value, catch via "const std::exception& ex") including location information with a user defined error message
        [[ noreturn ]] static void Throw(std::string_view reason, const std::source_location location = std::source_location::current());
        /// @brief Throws an exception (by value, catch via "const std::exception& ex") including location information with a user defined error message
        template <typename... Args>
        [[ noreturn ]] inline static void Throw(const std::source_location location, const char* const format, Args&&... args)
        {
            std::string reason = fmt::vformat(format, fmt::make_format_args(std::forward<Args>(args)...));
            Throw(reason, location);
        }

      protected:
        std::string mReason = std::string("");
    };

    /// @brief Asserts condition. Throws a generic error message if conditition is false.
    inline void Assert(bool condition, const std::source_location location = std::source_location::current())
    {
        if(!condition)
        {
            Exception::Throw("Assertion failed!", location);
        }
    }

    /// @brief Asserts condition. Throws a user defined error message if conditition is false.
    inline void Assert(bool condition, std::string_view message, const std::source_location location = std::source_location::current())
    {
        if(!condition)
        {
            Exception::Throw(message, location);
        }
    }
}  // namespace hsk

/// @brief Assertion macro for formatted error messages
/// @remark Why is this not a function? Many use cases would cause errors if format arguments were evaluated in a case where assertion passes. A macro circumvents this problem.
#define HSK_ASSERTFMT(val, fmt, ...)                                                                                                                                               \
    if(!(val))                                                                                                                                                                     \
    {                                                                                                                                                                              \
        const std::source_location __hsk_location = std::source_location::current();                                                                                                     \
        hsk::Exception::Throw(__hsk_location, fmt, __VA_ARGS__);                                                                                                                 \
    }

/// @brief Macro for throwing an exception with formatted error message argument
/// @remark Why is this not a function? Variadic functions don't mesh with default arguments (used to catch source location). A macro can circumvent this (albeit not very gracefully)
#define HSK_THROWFMT(fmt, ...)                                                                                                                                                     \
    {                                                                                                                                                                              \
        const std::source_location __hsk_location = std::source_location::current();                                                                                                     \
        hsk::Exception::Throw(__hsk_location, fmt, __VA_ARGS__);                                                                                                                 \
    }
