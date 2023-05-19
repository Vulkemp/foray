#pragma once
#include "../basics.hpp"
#include "../core/managedimage.hpp"
#include "../core/samplercollection.hpp"
#include "../mem.hpp"

namespace foray::util {
    /// @brief Uses std::mt19937_64 to generate a r32u image of decent quality random noise
    class NoiseSource : public core::ManagedResource
    {
      public:

        /// @brief Creates and uploads
        /// @param edge Width & Height
        /// @param depth Depth
        NoiseSource(core::Context* context, uint32_t edge = 2048U, uint32_t depth = 1);
        /// @brief Regenerates all values and uploads to texture
        virtual void Regenerate();

        virtual ~NoiseSource() = default;

        FORAY_GETTER_MEM(Image)

      protected:
        Local<core::ManagedImage> mImage;

    };
}  // namespace foray
