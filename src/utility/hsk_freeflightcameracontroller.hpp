#pragma once

#include "hsk_cameracontroller.hpp"
#include <unordered_map>

namespace hsk {

    class FreeFlightCameraController : public CameraController
    {
      public:
        FreeFlightCameraController()          = default;
        virtual ~FreeFlightCameraController() = default;

        virtual void Init(const VkContext* context, Camera* camera, OsManager* osManager) override;
        virtual void OnEvent(std::shared_ptr<Event> event) override;
        virtual void Update(float delta) override;

      protected:
        virtual void ProcessKeyboardEvent(std::shared_ptr<EventInputBinary> event);
        virtual void ProcessMouseMovedEvent(std::shared_ptr<EventInputMouseMoved> event);

        std::unordered_map<EButton, bool> mButtonsPressedMap;

        const InputBinary* mInputBinaryKeyW;
        const InputBinary* mInputBinaryKeyA;
        const InputBinary* mInputBinaryKeyS;
        const InputBinary* mInputBinaryKeyD;

        bool mIgnoreNextMouseEvent{false};

        

    };
}  // namespace hsk