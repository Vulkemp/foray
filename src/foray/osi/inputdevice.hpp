#pragma once
#include "helpers.hpp"
#include "input.hpp"
#include "osi_declares.hpp"
#include <memory>
#include <sdl2/SDL.h>
#include <string>
#include <vector>

namespace foray::osi {
    /// @brief Wraps a generic input device (mouse, keyboard, joystick, controller ...) in a hardware agnostic way
    class InputDevice
    {
      public:
        enum class EType : uint8_t
        {
            Unknown = 0,
            Mouse,
            Keyboard,
            Joystick
        };

        using AxisPtr = const InputAnalogue*;

        class AxisJoystick : public InputAnalogue
        {
          public:
            SDL_Joystick* Joystick;

            AxisJoystick() : InputAnalogue(), Joystick() {}
            AxisJoystick(InputDevice* device, int32_t id, std::string_view name, SDL_Joystick* joystick) : InputAnalogue(device, id, name, (EAxis)id), Joystick(joystick) {}

            // Inherited via Axis
            virtual int16_t State() const override;
        };

        class ButtonJoystick : public InputBinary
        {
          public:
            SDL_Joystick* Joystick;

            ButtonJoystick() : InputBinary(), Joystick() {}
            ButtonJoystick(InputDevice* device, int32_t id, std::string_view name, EButton button, SDL_Joystick* joystick)
                : InputBinary(device, id, name, button), Joystick(joystick)
            {
            }

            // Inherited via InputBinary
            virtual bool State() const override;
        };

        class ButtonMouse : public InputBinary
        {
          public:
            ButtonMouse() : InputBinary() {}
            ButtonMouse(InputDevice* device, int32_t id, std::string_view name, EButton button) : InputBinary(device, id, name, button) {}

            // Inherited via InputBinary
            virtual bool State() const override;
        };

        class ButtonKeyboard : public InputBinary
        {
          public:
            ButtonKeyboard() : InputBinary() {}
            ButtonKeyboard(InputDevice* device, int32_t id, std::string_view name, EButton button) : InputBinary(device, id, name, button) {}

            // Inherited via InputBinary
            virtual bool State() const override;
        };

        class DirectionalMouseScroll : public InputDirectional
        {
          public:
            inline DirectionalMouseScroll() : InputDirectional() {}
            inline DirectionalMouseScroll(InputDevice* device, int32_t id, std::string_view name, EDirectional directional) : InputDirectional(device, id, name, directional) {}
        };

        inline SDL_JoystickGUID    Guid() { return mGuid; }
        inline SDL_Joystick*       Joystick() { return mJoystick; }
        inline const SDL_Joystick* Joystick() const { return mJoystick; }
        inline SDL_JoystickID      JoystickId() { return mJoystickId; }

        InputDevice()
            : mGuid(), mJoystick(), mJoystickId(-1), mJoystickButtons(), mJoystickAxes(), mKeyboardButtons(), mMouseButtons(), mName(), mType(EType::Unknown), mAxes(), mButtons()
        {
        }
        InputDevice(const InputDevice& other)            = delete;
        InputDevice(const InputDevice&& other)           = delete;
        InputDevice& operator=(const InputDevice& other) = delete;

        const std::vector<AxisPtr>&           Axes() const { return mAxes; }
        const std::vector<InputBinary*>&      Buttons() const { return mButtons; }
        const std::vector<InputDirectional*>& Directionals() const { return mDirectionals; }

        const std::string& Name() const { return mName; }
        EType              Type() const { return mType; }

        /// @brief Finds a button object based on the corresponding enum value.
        /// InputBinary objects are valid until the input device is deleted, so storing the object for later use is a good idea.
        /// @return nullptr if not found, a valid button object
        const InputBinary* FindButton(EButton button) const;

        std::string BuildDebugPrint() const;

        static InputDevice* InitKeyboard(std::vector<std::unique_ptr<InputDevice>>& out);
        static InputDevice* InitMouse(std::vector<std::unique_ptr<InputDevice>>& out);
        static InputDevice* InitJoystick(std::vector<std::unique_ptr<InputDevice>>& out, SDL_Joystick* joystick);

        virtual ~InputDevice();

      protected:
        SDL_JoystickGUID mGuid;
        SDL_Joystick*    mJoystick;
        SDL_JoystickID   mJoystickId;

        ButtonJoystick*        mJoystickButtons;
        AxisJoystick*          mJoystickAxes;
        ButtonKeyboard*        mKeyboardButtons;
        ButtonMouse*           mMouseButtons;
        DirectionalMouseScroll mMouseScroll;

        std::string mName;
        EType       mType;

        std::vector<AxisPtr>           mAxes;
        std::vector<InputBinary*>      mButtons;
        std::vector<InputDirectional*> mDirectionals;
    };

}  // namespace foray