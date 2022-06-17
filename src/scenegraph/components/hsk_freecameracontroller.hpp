#pragma once
#include "../../hsk_glm.hpp"
#include "../../osi/hsk_event.hpp"
#include "../hsk_component.hpp"

namespace hsk {
    class FreeCameraController : public NodeComponent, public Component::UpdateCallback, public Component::OnEventCallback
    {
      public:
        virtual void OnEvent(std::shared_ptr<Event>& event) override;

        virtual void Update(const FrameUpdateInfo&) override;

      protected:
        struct InputStates
        {
            bool Forward;
            bool Backward;
            bool StrafeUp;
            bool StrafeDown;
            bool StrafeLeft;
            bool StrafeRight;
        } mInputStates = {};

        int mSpeedExponent = 0;

        std::map<EButton, bool&> mMapping = {{EButton::Keyboard_W, mInputStates.Forward},       {EButton::Keyboard_S, mInputStates.Backward},
                                             {EButton::Keyboard_LShift, mInputStates.StrafeUp}, {EButton::Keyboard_LCtrl, mInputStates.StrafeDown},
                                             {EButton::Keyboard_A, mInputStates.StrafeLeft},    {EButton::Keyboard_D, mInputStates.Forward}};
    };
}  // namespace hsk