#pragma once
#include "../../foray_glm.hpp"
#include "../foray_component.hpp"

namespace foray::scene::ncomp {

    /// @brief Defines a nodes transform relative to its parent (or world origin, if no parent is set)
    class Transform : public NodeComponent
    {
      public:
        inline Transform() {}

        FORAY_PROPERTY_R(Translation)
        FORAY_PROPERTY_R(Rotation)
        FORAY_PROPERTY_R(Scale)
        FORAY_PROPERTY_R(LocalMatrix)
        FORAY_PROPERTY_V(Static)
        FORAY_GETTER_CR(GlobalMatrix)
        FORAY_PROPERTY_V(LocalMatrixFixed)

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
}  // namespace foray