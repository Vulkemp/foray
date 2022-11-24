#pragma once
#include "../../foray_glm.hpp"
#include "../foray_component.hpp"

namespace foray::scene::ncomp {

    /// @brief Defines a nodes transform relative to its parent (or world origin, if no parent is set)
    class Transform : public NodeComponent
    {
      public:
        inline Transform() {}

        FORAY_GETTER_R(Translation)
        FORAY_GETTER_R(Rotation)
        FORAY_GETTER_R(Scale)
        FORAY_GETTER_R(LocalMatrix)
        FORAY_PROPERTY_V(Static)
        FORAY_PROPERTY_V(LocalMatrixFixed)
        FORAY_GETTER_V(Dirty)

        const glm::mat4& GetGlobalMatrix();

        void RecalculateIfDirty(bool recursive = false);

        void SetTranslation(const glm::vec3& translation);
        void SetRotation(const glm::quat& rotation);
        void SetScale(const glm::vec3& scale);
        void SetLocalMatrix(const glm::mat4& matrix);

      protected:
        void SetDirtyRecursively();
        void RecalculateLocalMatrix();
        void RecalculateGlobalMatrix();


        glm::vec3 mTranslation      = {};
        glm::quat mRotation         = {};
        glm::vec3 mScale            = glm::vec3(1.f);
        glm::mat4 mLocalMatrix      = glm::mat4(1.f);
        glm::mat4 mGlobalMatrix     = glm::mat4(1.f);
        bool      mStatic           = false;
        bool      mLocalMatrixFixed = false;
        bool      mDirty            = true;
    };
}  // namespace foray::scene::ncomp