#pragma once
#include "../../foray_glm.hpp"
#include "../../osi/foray_event.hpp"
#include "../foray_component.hpp"
#include <unordered_map>

namespace foray::scene::ncomp {

    /// @brief A simple camera controller for free flight
    /// @details
    /// Controls:
    ///  * W/S                      Forwards/Backwards
    ///  * A/D Strafe               Left/Right
    ///  * LCtrl/LShift Strafe      Up/Down
    ///  * ScrollWheel/Numpad+/-    Increase / decrease movement speed (exponentially)
    ///  * Arrow Keys               Pitch & Yaw
    ///  * Space                    Toggle mouse capture for pitch & yaw
    class FreeCameraController : public NodeComponent, public Component::UpdateCallback, public Component::OnEventCallback
    {
      public:
        virtual void OnEvent(const osi::Event* event) override;

        virtual void           Update(SceneUpdateInfo&) override;
        inline virtual int32_t GetOrder() const override { return 0; }

        FORAY_PROPERTY_V(InvertYAxis)
        FORAY_PROPERTY_V(InvertAll)

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

        bool mInvertYAxis = false;
        bool mInvertAll   = false;

        std::unordered_map<osi::EButton, bool&> mMapping = {{osi::EButton::Keyboard_W, mInputStates.Forward},       {osi::EButton::Keyboard_S, mInputStates.Backward},
                                                            {osi::EButton::Keyboard_LShift, mInputStates.StrafeUp}, {osi::EButton::Keyboard_LCtrl, mInputStates.StrafeDown},
                                                            {osi::EButton::Keyboard_A, mInputStates.StrafeLeft},    {osi::EButton::Keyboard_D, mInputStates.StrafeRight},
                                                            {osi::EButton::Keyboard_Up, mInputStates.PitchUp},      {osi::EButton::Keyboard_Down, mInputStates.PitchDown},
                                                            {osi::EButton::Keyboard_Left, mInputStates.YawLeft},    {osi::EButton::Keyboard_Right, mInputStates.YawRight}};
    };
}  // namespace foray::scene::ncomp