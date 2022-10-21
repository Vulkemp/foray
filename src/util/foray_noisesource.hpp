#pragma once
#include "../foray_basics.hpp"
#include "../core/foray_managedimage.hpp"

namespace foray::util {
    class NoiseSource : public core::DeviceResourceBase
    {
      public:
        NoiseSource();

        void         Create(core::Context* context);
        virtual void Destroy() override;
        virtual bool Exists() const override;

        FORAY_PROPERTY_ALLGET(Image)

      protected:
        core::ManagedImage mImage;

    };
}  // namespace foray
