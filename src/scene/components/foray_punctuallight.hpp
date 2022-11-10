#pragma once
#include "../../foray_glm.hpp"
#include "../foray_component.hpp"
#include "../foray_scene_declares.hpp"
#include "../foray_lights.hpp"

namespace foray::scene::ncomp {
    /// @brief Defines a simple punctual light (directional or point)
    class PunctualLight : public NodeComponent
    {
      public:
        FORAY_PROPERTY_V(Type)
        FORAY_PROPERTY_R(Color)
        FORAY_PROPERTY_V(Intensity)

        void UpdateStruct(SimpleLight& simplifiedlight);

      protected:
        ELightType mType      = ELightType::Point;
        glm::vec3  mColor     = glm::vec3(1.f);
        fp32_t     mIntensity = 1.f;
    };
}  // namespace foray::scene