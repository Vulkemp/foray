#pragma once
#include "../hsk_basics.hpp"
#include <vulkan/vulkan.h>

namespace hsk {

    /// @brief Base class enforcing common interface for all classes wrapping a device resource
    class DeviceResourceBase : public NoMoveDefaults
    {
      public:
        virtual bool Exists() const = 0;
        virtual void Cleanup()      = 0;
        inline virtual ~DeviceResourceBase() {}

        std::string_view GetName() const { return mName; }

        inline virtual DeviceResourceBase& SetName(std::string_view name);

      protected:
        std::string mName = {};
    };

    inline DeviceResourceBase& DeviceResourceBase::SetName(std::string_view name)
    {
        mName = name;
        return *this;
    }
}  // namespace hsk