#pragma once
#include "../../foray_glm.hpp"
#include "../foray_component.hpp"
#include "../foray_scene_declares.hpp"
#include "../foray_simplifiedlight.hpp"

namespace foray::scene {
    class SimplifiedLightComponent : public NodeComponent
    {
      public:
        enum class EType : uint32_t
        {
            Directional = 0,
            Point       = 1
        };

        FORAY_PROPERTY_ALL(Type)
        FORAY_PROPERTY_ALL(RadiantFlux)

        void UpdateStruct(SimplifiedLight& simplifiedlight);

      protected:
        EType     mType        = EType::Point;
        glm::vec3 mRadiantFlux = glm::vec3(10);
    };
}  // namespace foray