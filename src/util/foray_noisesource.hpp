#pragma once
#include "../foray_basics.hpp"
#include "../core/foray_managedimage.hpp"
#include "../core/foray_samplercollection.hpp"

namespace foray::util {
    class NoiseSource : public core::ManagedResource
    {
      public:
        NoiseSource();

        void         Create(core::Context* context);
        virtual void Destroy() override;
        virtual bool Exists() const override;

        FORAY_PROPERTY_ALLGET(Image)
        FORAY_PROPERTY_ALLGET(Sampler)

      protected:
        core::ManagedImage mImage;
        core::CombinedImageSampler mSampler;

    };
}  // namespace foray
