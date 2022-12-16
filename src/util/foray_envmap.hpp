#pragma once
#include "../core/foray_managedimage.hpp"
#include "../core/foray_samplercollection.hpp"
#include "foray_util_declares.hpp"
#include "../osi/foray_env.hpp"

namespace foray::util {

    /// @brief Experimental type loading an environment map in spherical representation, also generates mip maps
    class EnvironmentMap
    {
      public:
        void Create(core::Context* context, const osi::Utf8Path& path, std::string_view name, VkFormat loadFormat = VkFormat::VK_FORMAT_UNDEFINED, VkFormat storeFormat = VkFormat::VK_FORMAT_R16G16B16A16_SFLOAT);
        void Destroy();

        virtual ~EnvironmentMap() {Destroy();}

        FORAY_GETTER_CR(Sampler)
        FORAY_GETTER_CR(Image)

      protected:
        core::ManagedImage mImage;
        core::SamplerReference mSampler;
    };
}  // namespace foray::util
