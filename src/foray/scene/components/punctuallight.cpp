#include "punctuallight.hpp"
#include "../node.hpp"
#include "transform.hpp"
#include <glm/gtx/matrix_decompose.hpp>

namespace foray::scene::ncomp {
    void PunctualLight::UpdateStruct(SimpleLight& simplifiedlight)
    {
        simplifiedlight.Color     = mColor;
        simplifiedlight.Intensity = mIntensity;
        simplifiedlight.Type      = mType;

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
            case ELightType::Directional: {
                glm::vec4 dir(0.f, 0.f, 1.f, 1.f);
                dir                      = glm::mat4(rot) * dir;
                simplifiedlight.PosOrDir = dir;
                break;
            }
            case ELightType::Point:
            default: {
                simplifiedlight.PosOrDir = pos;
                break;
            }
        }
    }
}  // namespace foray::scene::ncomp
