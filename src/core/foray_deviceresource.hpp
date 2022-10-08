#pragma once
#include "../foray_basics.hpp"
#include "../foray_vulkan.hpp"
#include <unordered_set>

namespace foray::core {

    /// @brief Base class enforcing common interface for all classes wrapping a device resource
    class DeviceResourceBase : public NoMoveDefaults
    {
        /// @brief Every allocation registers with its name, so when clearing up, we can track if there are unfreed resources left.
        static inline std::unordered_set<DeviceResourceBase*> sAllocatedRessources{};

      public:
        static const std::unordered_set<DeviceResourceBase*>* GetTotalAllocatedResources() { return &sAllocatedRessources; }

        virtual bool Exists() const = 0;
        virtual void Destroy()      = 0;

        inline DeviceResourceBase() { sAllocatedRessources.insert(this); }
        inline explicit DeviceResourceBase(std::string_view name);
        inline virtual ~DeviceResourceBase() { sAllocatedRessources.erase(this); }

        std::string_view                   GetName() const { return mName; }
        inline virtual DeviceResourceBase& SetName(std::string_view name);

      protected:
        std::string mName;
    };

    inline DeviceResourceBase::DeviceResourceBase(std::string_view name)
    {
        mName = name;
        sAllocatedRessources.insert(this);
    }

    inline DeviceResourceBase& DeviceResourceBase::SetName(std::string_view name)
    {
        mName = name;
        return *this;
    }
}  // namespace foray::core