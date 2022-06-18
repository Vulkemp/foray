#include "hsk_freecameracontroller.hpp"
#include "../hsk_node.hpp"
#include "hsk_camera.hpp"

namespace hsk {
    void FreeCameraController::OnEvent(std::shared_ptr<Event>& event)
    {
        auto binaryInputEvent = std::dynamic_pointer_cast<EventInputBinary>(event);
        if(binaryInputEvent)
        {
            auto finditer = mMapping.find(binaryInputEvent->Button()->Button());
            if(finditer != mMapping.end())
            {
                finditer->second = binaryInputEvent->Pressed();
            }

            if(binaryInputEvent->Button()->Button() == EButton::Keyboard_Numpad_Plus && binaryInputEvent->Pressed())
            {
                mSpeedExponent++;
            }
            if(binaryInputEvent->Button()->Button() == EButton::Keyboard_Numpad_Minus && binaryInputEvent->Pressed())
            {
                mSpeedExponent--;
            }
            if(binaryInputEvent->Button()->Button() == EButton::Keyboard_Space && binaryInputEvent->Pressed())
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

        auto directional = std::dynamic_pointer_cast<EventInputDirectional>(event);
        if(directional && directional->InputSource()->GetDirectional() == EDirectional::Mouse_Scroll)
        {
            mSpeedExponent += std::clamp(directional->OffsetY(), -1, 1);
        }

        auto mouseMoved = std::dynamic_pointer_cast<EventInputMouseMoved>(event);
        if(mouseMoved && mUseMouse)
        {
            ProcessMouseMovedEvent(mouseMoved);
        }
    }

    void FreeCameraController::Update(const FrameUpdateInfo& updateInfo)
    {
        Camera* camera = GetNode()->GetComponent<Camera>();
        if(!camera)
        {
            return;
        }

        float deltaTime = updateInfo.GetFrameTime();
        float speed     = exp2f(mSpeedExponent) * deltaTime;
        auto& pos       = camera->GetEyePosition();
        auto& lookAt    = camera->GetLookatPosition();
        auto& upDir     = camera->GetUpDirection();

        if(mInputStates.PitchUp)
            mPitch += glm::radians(-1.f * KEYBOARD_ROTATION_SENSIBILITY * deltaTime);
        if(mInputStates.PitchDown)
            mPitch += glm::radians(KEYBOARD_ROTATION_SENSIBILITY * deltaTime);
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
            pos += -1.f * speed * upDir;
        if(mInputStates.StrafeDown)
            pos += speed * upDir;


        lookAt = pos + lookDir;
        camera->SetViewMatrix();
    }

    void FreeCameraController::ProcessMouseMovedEvent(std::shared_ptr<EventInputMouseMoved>& event)
    {
        Camera* camera = GetNode()->GetComponent<Camera>();
        if(!camera)
        {
            return;
        }

        float xoffset = MOUSE_ROTATION_SENSIBILITY * event->RelativeX();
        float yoffset = MOUSE_ROTATION_SENSIBILITY * event->RelativeY();

        mYaw += xoffset;
        mPitch += yoffset * (mInvertYAxis ? -1.f : 1.f);

        // make sure that when pitch is out of bounds, screen doesn't get flipped
        mPitch = std::clamp(mPitch, -89.f, 89.f);

        glm::vec3 lookDir(std::cos(glm::radians(mYaw)) * std::cos(glm::radians(mPitch)), std::sin(glm::radians(mPitch)),
                          std::sin(glm::radians(mYaw)) * std::cos(glm::radians(mPitch)));
        auto&     pos    = camera->GetEyePosition();
        auto&     lookAt = camera->GetLookatPosition();
        lookAt           = pos + lookDir;
        camera->SetViewMatrix();
    }


}  // namespace hsk