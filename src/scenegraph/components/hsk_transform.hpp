#pragma once
#include "../hsk_component.hpp"
#include <glm/ext.hpp>
#include <glm/glm.hpp>

namespace hsk {
    class NTransform : public Component
    {
      public:
        inline NTransform() {}

        HSK_PROPERTY_ALL(Translation)
        HSK_PROPERTY_ALL(Rotation)
        HSK_PROPERTY_ALL(Scale)
        HSK_PROPERTY_ALL(LocalMatrix)
        HSK_PROPERTY_CGET(GlobalMatrix)

        void RecalculateLocalMatrix();
        void RecalculateGlobalMatrix(NTransform* parentTransform);

      protected:
        glm::vec3 mTranslation  = {};
        glm::quat mRotation     = {};
        glm::vec3 mScale        = glm::vec3(1.f);
        glm::mat4 mLocalMatrix  = glm::mat4(1.f);
        glm::mat4 mGlobalMatrix = glm::mat4(1.f);
    };
}  // namespace hsk