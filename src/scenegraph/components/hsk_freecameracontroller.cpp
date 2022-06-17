#include "hsk_freecameracontroller.hpp"
#include "hsk_camera.hpp"
#include "../hsk_node.hpp"

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
        }

        auto mouseMoved = std::dynamic_pointer_cast<EventInputMouseMoved>(event);
        if(mouseMoved)
        {
            // ProcessMouseMovedEvent(mouseMoved);
        }
    }

    void FreeCameraController::Update(const FrameUpdateInfo& updateInfo)
    {
        NCamera* camera = GetNode()->GetComponent<NCamera>();
        if (!camera){
            return;
        }

        float deltaTime = updateInfo.GetFrameTime();
        float speed     = exp2f(mSpeedExponent) * deltaTime;
        auto& pos = camera->GetEyePosition();
        auto& lookAt = camera->GetLookatPosition();
        auto& upDir = camera->GetUpDirection();

        auto lookDir = glm::normalize(lookAt - pos);

        if(mInputStates.Forward)
            pos += speed * lookDir;
        if(mInputStates.Backward)
            pos += -1.f * speed * lookDir;
        if(mInputStates.StrafeLeft)
            pos += speed * glm::normalize(glm::cross(lookDir, upDir));
        if(mInputStates.StrafeRight)
            pos += -1.f * speed * glm::normalize(glm::cross(lookDir, upDir));
        if(mInputStates.StrafeUp)
            pos += speed * upDir;
        if(mInputStates.StrafeDown)
            pos += speed * upDir;


        lookAt = pos + lookDir;
        camera->SetViewMatrix();

        logger()->info("Camera pos: ({}, {}, {})", pos.x, pos.y, pos.z);
    }

}  // namespace hsk