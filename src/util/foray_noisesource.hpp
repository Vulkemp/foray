#pragma once
#include "../foray_basics.hpp"
#include "../core/foray_managedimage.hpp"
#include "../core/foray_samplercollection.hpp"

namespace foray::util {
    /// @brief Uses std::mt19937_64 to generate a r32u image of decent quality random noise
    class NoiseSource : public core::ManagedResource
    {
      public:
        NoiseSource() = default;

        /// @brief Creates and uploads
        /// @param edge Width & Height
        /// @param depth Depth
        void         Create(core::Context* context, uint32_t edge = 2048U, uint32_t depth = 1);
        /// @brief Regenerates all values and uploads to texture
        virtual void Regenerate();
        virtual void Destroy() override;
        virtual bool Exists() const override;

        FORAY_GETTER_R(Image)

      protected:
        core::ManagedImage mImage;

    };
}  // namespace foray
