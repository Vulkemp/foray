#pragma once
#include "../../hsk_glm.hpp"
#include "../hsk_component.hpp"
#include "../hsk_scenegraph_declares.hpp"
#include "../hsk_simplifiedlight.hpp"

namespace hsk {
    class SimplifiedLightComponent : public NodeComponent
    {
      public:
        enum class EType : uint32_t
        {
            Directional = 0,
            Point       = 1
        };

        HSK_PROPERTY_ALL(Type)
        HSK_PROPERTY_ALL(RadiantFlux)

        void UpdateStruct(SimplifiedLight& simplifiedlight);

      protected:
        EType     mType        = EType::Point;
        glm::vec3 mRadiantFlux = glm::vec3(10);
    };
}  // namespace hsk