#pragma once
#include <string>
#include <exception>

namespace hsk
{
    class Exception : std::exception
    {
    public:
        const std::string Reason;

        Exception(const std::string &reason) : Reason(reason) {}
        virtual ~Exception() {}

        inline virtual const char *what() const noexcept override { return Reason.data(); }
    };
}