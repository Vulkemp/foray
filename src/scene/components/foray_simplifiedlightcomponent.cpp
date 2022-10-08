#include "foray_simplifiedlightcomponent.hpp"
#include "../foray_node.hpp"
#include "foray_transform.hpp"
#include <glm/gtx/matrix_decompose.hpp>

namespace foray::scene {
    void SimplifiedLightComponent::UpdateStruct(SimplifiedLight& simplifiedlight)
    {
        simplifiedlight.RadiantFluxRgb = mRadiantFlux;
        simplifiedlight.Type           = (uint32_t)mType;

        glm::mat4 globalMat = GetNode()->GetTransform()->GetGlobalMatrix();
        glm::vec3 scale;
        glm::vec3 pos;
        glm::quat rot;
        glm::vec3 skew;
        glm::vec4 perspective;

        if(!glm::decompose(globalMat, scale, rot, pos, skew, perspective))
        {
            pos = glm::vec3();
            rot = glm::quat();
        }

        switch(mType)
        {
            case EType::Directional: {
                glm::vec4 dir(0.f, 0.f, 1.f, 1.f);
                dir                      = glm::mat4(rot) * dir;
                simplifiedlight.PosOrDir = dir;
                break;
            }
            case EType::Point:
            default: {
                simplifiedlight.PosOrDir = pos;
                break;
            }
        }
    }
}  // namespace foray::scene
