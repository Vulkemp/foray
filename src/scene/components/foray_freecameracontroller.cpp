#include "foray_freecameracontroller.hpp"
#include "../../foray_logger.hpp"
#include "../foray_node.hpp"
#include "foray_camera.hpp"
#include "foray_transform.hpp"

namespace foray::scene::ncomp {
    void FreeCameraController::OnOsEvent(const osi::Event* event)
    {
        auto binaryInputEvent = dynamic_cast<const osi::EventInputBinary*>(event);
        if(binaryInputEvent)
        {
            auto buttonId = binaryInputEvent->SourceInput->GetButtonId();
            auto pressed  = binaryInputEvent->State;

            auto finditer = mMapping.find(buttonId);
            if(finditer != mMapping.end())
            {
                finditer->second = pressed;
            }

            if(buttonId == osi::EButton::Keyboard_Numpad_Plus && pressed)
            {
                mSpeedExponent++;
            }
            if(buttonId == osi::EButton::Keyboard_Numpad_Minus && pressed)
            {
                mSpeedExponent--;
            }
            if(buttonId == osi::EButton::Keyboard_Space && pressed)
            {
                int code = SDL_SetRelativeMouseMode(mUseMouse ? SDL_FALSE : SDL_TRUE);
                if(code < 0)
                {
                    logger()->warn("SDL relative mouse not supported: {}", SDL_GetError());
                }
                else
                {
                    mUseMouse = !mUseMouse;
                }
            }
        }

        auto directional = dynamic_cast<const osi::EventInputDirectional*>(event);
        if(directional && directional->SourceInput->GetDirectionalId() == osi::EDirectional::Mouse_Scroll)
        {
            mSpeedExponent += std::clamp(directional->OffsetY, -1, 1);
        }

        auto mouseMoved = dynamic_cast<const osi::EventInputMouseMoved*>(event);
        if(mouseMoved && mUseMouse)
        {
            ProcessMouseMovedEvent(mouseMoved);
        }
    }

    void FreeCameraController::Update(SceneUpdateInfo& updateInfo)
    {
        Camera* camera = GetNode()->GetComponent<Camera>();
        Transform* transform = GetNode()->GetTransform();
        if(!camera)
        {
            return;
        }

        fp32_t invertMulti = mInvertAll ? -1.f : 1.f;

        float deltaTime = updateInfo.RenderInfo.GetFrameTime();
        float speed     = exp2f((fp32_t)mSpeedExponent) * deltaTime;
        glm::vec3& pos       = transform->GetTranslation();
        glm::vec3& upDir     = camera->GetUpDirection();

        if(mInputStates.PitchUp)
            mPitch += glm::radians(-1.f * KEYBOARD_ROTATION_SENSIBILITY * deltaTime) * invertMulti;
        if(mInputStates.PitchDown)
            mPitch += glm::radians(KEYBOARD_ROTATION_SENSIBILITY * deltaTime) * invertMulti;
        if(mInputStates.YawLeft)
            mYaw += glm::radians(-1.f * KEYBOARD_ROTATION_SENSIBILITY * deltaTime);
        if(mInputStates.YawRight)
            mYaw += glm::radians(KEYBOARD_ROTATION_SENSIBILITY * deltaTime);

        mPitch = std::clamp(mPitch, -89.f, 89.f);

        glm::vec3 lookDir(std::cos(glm::radians(mYaw)) * std::cos(glm::radians(mPitch)), std::sin(glm::radians(mPitch)),
                          std::sin(glm::radians(mYaw)) * std::cos(glm::radians(mPitch)));

        if(mInputStates.Forward)
            pos += speed * lookDir;
        if(mInputStates.Backward)
            pos += -1.f * speed * lookDir;
        if(mInputStates.StrafeLeft)
            pos += -1.f * speed * glm::normalize(glm::cross(lookDir, upDir));
        if(mInputStates.StrafeRight)
            pos += speed * glm::normalize(glm::cross(lookDir, upDir));
        if(mInputStates.StrafeUp)
            pos += speed * upDir;
        if(mInputStates.StrafeDown)
            pos += -1.f * speed * upDir;

        transform->SetTranslation(pos);
        transform->SetRotation(glm::quatLookAt(lookDir, camera->GetUpDirection()));
    }

    void FreeCameraController::ProcessMouseMovedEvent(const osi::EventInputMouseMoved* event)
    {
        Camera* camera = GetNode()->GetComponent<Camera>();
        Transform* transform = GetNode()->GetTransform();
        if(!camera)
        {
            return;
        }

        fp32_t invertMulti = mInvertAll ? -1.f : 1.f;

        float xoffset = MOUSE_ROTATION_SENSIBILITY * event->RelativeX;
        float yoffset = MOUSE_ROTATION_SENSIBILITY * event->RelativeY;

        mYaw += xoffset;
        mPitch += yoffset * (mInvertYAxis ? -1.f : 1.f) * invertMulti;

        // make sure that when pitch is out of bounds, screen doesn't get flipped
        mPitch = std::clamp(mPitch, -89.f, 89.f);

        glm::vec3 lookDir(std::cos(glm::radians(mYaw)) * std::cos(glm::radians(mPitch)), std::sin(glm::radians(mPitch)),
                          std::sin(glm::radians(mYaw)) * std::cos(glm::radians(mPitch)));

        transform->SetRotation(glm::quatLookAt(lookDir, camera->GetUpDirection()));
    }
}  // namespace foray::scene