#pragma once
#include "../../foray_glm.hpp"
#include "../../osi/foray_event.hpp"
#include "../foray_component.hpp"
#include <map>

namespace foray::scene {
    class FreeCameraController : public NodeComponent, public Component::UpdateCallback, public Component::OnEventCallback
    {
      public:
        virtual void OnEvent(const Event* event) override;

        virtual void           Update(SceneUpdateInfo&) override;
        inline virtual int32_t GetOrder() const override { return 0; }

      protected:
        virtual void ProcessMouseMovedEvent(const EventInputMouseMoved* event);

        struct InputStates
        {
            bool Forward;
            bool Backward;
            bool StrafeUp;
            bool StrafeDown;
            bool StrafeLeft;
            bool StrafeRight;
            bool PitchUp;
            bool PitchDown;
            bool YawRight;
            bool YawLeft;
        } mInputStates = {};

        int mSpeedExponent = 0;

        inline static constexpr float KEYBOARD_ROTATION_SENSIBILITY = 9000.f;
        float                         mPitch                        = 0;
        float                         mYaw                          = 0;
        inline static constexpr float MOUSE_ROTATION_SENSIBILITY    = .05f;
        bool                          mUseMouse                     = false;
        bool                          mInvertYAxis                  = false;

        std::map<EButton, bool&> mMapping = {{EButton::Keyboard_W, mInputStates.Forward},       {EButton::Keyboard_S, mInputStates.Backward},
                                             {EButton::Keyboard_LShift, mInputStates.StrafeUp}, {EButton::Keyboard_LCtrl, mInputStates.StrafeDown},
                                             {EButton::Keyboard_A, mInputStates.StrafeLeft},    {EButton::Keyboard_D, mInputStates.StrafeRight},
                                             {EButton::Keyboard_Up, mInputStates.PitchUp},      {EButton::Keyboard_Down, mInputStates.PitchDown},
                                             {EButton::Keyboard_Left, mInputStates.YawLeft},    {EButton::Keyboard_Right, mInputStates.YawRight}};
    };
}  // namespace foray::scene