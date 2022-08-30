#pragma once
#include "../hsk_basics.hpp"
#include "../memory/hsk_managedimage.hpp"

namespace hsk {
    class NoiseSource : public DeviceResourceBase
    {
      public:
        NoiseSource();

        void         Create(const VkContext* context);
        virtual void Destroy() override;
        virtual bool Exists() const override;

        HSK_PROPERTY_ALLGET(Image)

      protected:
        ManagedImage mImage;

    };
}  // namespace hsk
