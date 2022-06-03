#pragma once
#include "../glTF/hsk_camera.hpp"
#include "../osi/hsk_event.hpp"
#include <glm/glm.hpp>
#include "../base/hsk_vkcontext.hpp"

namespace hsk {
    class CameraController
    {
      public:
        CameraController()          = default;
        virtual ~CameraController() = default;

        virtual void Init(const VkContext* context, Camera* camera, OsManager* osManager);

        virtual void OnEvent(std::shared_ptr<Event> event);

        virtual void Update(float delta){};

        HSK_PROPERTY_ALL(CameraActive)
        HSK_PROPERTY_ALL(ReactOnMouseMoveEvents)

      protected:
        const VkContext* mContext;
        /// @brief The camera that camera controller modifies.
        Camera*    mCamera{};
        OsManager* mOsManager{};

        bool mCameraActive{true};
        bool mReactOnMouseMoveEvents{false};

        glm::vec3 mCameraPos    = glm::vec3(0.0f, 0.0f, 0.0f);
        glm::vec3 mCameraFront  = glm::vec3(0.0f, 0.0f, -1.0f);
        glm::vec3 mCameraUp     = glm::vec3(0.0f, 1.0f, 0.0f);
        float     mCameraSpeed  = 0.002f;
        float     mInvertYAxis = -1;

        glm::mat4  mViewMatrix;
        glm::mat4  mProjectionMatrix;
        glm::mat4* mViewProjMatrix;

        bool  mFirstMoveAfterUnlock = true;
        float mYaw =
            -90.0f;  // yaw is initialized to -90.0 degrees since a yaw of 0.0 results in a direction vector pointing to the right so we initially rotate a bit to the left.
        float       mPitch       = 0.0f;
        float       mSensitivity = 0.1;

        bool mMoveForwards{false};
        bool mMoveBackwards{false};
        bool mMoveLeft{false};
        bool mMoveRight{false};

    };
}  // namespace hsk