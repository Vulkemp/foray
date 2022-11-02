#pragma once
#include "../../foray_glm.hpp"
#include "../../osi/foray_event.hpp"
#include "../foray_component.hpp"
#include <unordered_map>

namespace foray::scene::ncomp {
    class FreeCameraController : public NodeComponent, public Component::UpdateCallback, public Component::OnEventCallback
    {
      public:
        virtual void OnEvent(const osi::Event* event) override;

        virtual void           Update(SceneUpdateInfo&) override;
        inline virtual int32_t GetOrder() const override { return 0; }

      protected:
        virtual void ProcessMouseMovedEvent(const osi::EventInputMouseMoved* event);

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

        std::unordered_map<osi::EButton, bool&> mMapping = {{osi::EButton::Keyboard_W, mInputStates.Forward},       {osi::EButton::Keyboard_S, mInputStates.Backward},
                                                  {osi::EButton::Keyboard_LShift, mInputStates.StrafeUp}, {osi::EButton::Keyboard_LCtrl, mInputStates.StrafeDown},
                                                  {osi::EButton::Keyboard_A, mInputStates.StrafeLeft},    {osi::EButton::Keyboard_D, mInputStates.StrafeRight},
                                                  {osi::EButton::Keyboard_Up, mInputStates.PitchUp},      {osi::EButton::Keyboard_Down, mInputStates.PitchDown},
                                                  {osi::EButton::Keyboard_Left, mInputStates.YawLeft},    {osi::EButton::Keyboard_Right, mInputStates.YawRight}};
    };
}  // namespace foray::scene