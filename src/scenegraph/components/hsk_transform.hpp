#pragma once
#include "../../hsk_glm.hpp"
#include "../hsk_component.hpp"

namespace hsk {
    class Transform : public NodeComponent
    {
      public:
        inline Transform() {}

        HSK_PROPERTY_ALL(Translation)
        HSK_PROPERTY_ALL(Rotation)
        HSK_PROPERTY_ALL(Scale)
        HSK_PROPERTY_ALL(LocalMatrix)
        HSK_PROPERTY_ALL(Static)
        HSK_PROPERTY_CGET(GlobalMatrix)
        HSK_PROPERTY_ALL(LocalMatrixFixed)

        void RecalculateLocalMatrix();
        void RecalculateGlobalMatrix(Transform* parentTransform = nullptr);

      protected:
        glm::vec3 mTranslation      = {};
        glm::quat mRotation         = {};
        glm::vec3 mScale            = glm::vec3(1.f);
        glm::mat4 mLocalMatrix      = glm::mat4(1.f);
        glm::mat4 mGlobalMatrix     = glm::mat4(1.f);
        bool      mStatic           = false;
        bool      mLocalMatrixFixed = false;
    };
}  // namespace hsk