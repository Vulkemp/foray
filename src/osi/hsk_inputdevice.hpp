#pragma once
#include "hsk_osi_declares.hpp"
#include "hsk_helpers.hpp"
#include "hsk_input.hpp"
#include "../hsk_memory.hpp"
#include <string>
#include <vector>
#include <memory>
#include <sdl2/SDL.h>

namespace hsk
{
    /// @brief Wraps a generic input device (mouse, keyboard, joystick, controller ...) in a hardware agnostic way
    class InputDevice
    {
    public:
        using loanptr = loan_ptr<InputDevice>;
        using uniqueptr = std::unique_ptr<InputDevice>;

        enum class EType : uint8_t
        {
            Unknown = 0,
            Mouse,
            Keyboard,
            Joystick
        };

        using AxisPtr = loan_ptr<const InputAnalogue>;
        using ButtonPtr = loan_ptr<const InputBinary>;

        class AxisJoystick : public InputAnalogue
        {
        public:
            SDL_Joystick *Joystick;

            AxisJoystick() : InputAnalogue(), Joystick() {}
            AxisJoystick(loan_ptr<InputDevice> device, int32_t id, std::string_view name, SDL_Joystick *joystick) : InputAnalogue(device, id, name, (EAxis)id), Joystick(joystick) {}

            // Inherited via Axis
            virtual int16_t State() const override;
        };

        class ButtonJoystick : public InputBinary
        {
        public:
            SDL_Joystick *Joystick;

            ButtonJoystick() : InputBinary(), Joystick() {}
            ButtonJoystick(loan_ptr<InputDevice> device, int32_t id, std::string_view name, EButton button, SDL_Joystick *joystick) : InputBinary(device, id, name, button), Joystick(joystick) {}

            // Inherited via InputBinary
            virtual bool State() const override;
        };

        class ButtonMouse : public InputBinary
        {
        public:
            ButtonMouse() : InputBinary() {}
            ButtonMouse(loan_ptr<InputDevice> device, int32_t id, std::string_view name, EButton button) : InputBinary(device, id, name, button) {}

            // Inherited via InputBinary
            virtual bool State() const override;
        };

        class ButtonKeyboard : public InputBinary
        {
        public:
            ButtonKeyboard() : InputBinary() {}
            ButtonKeyboard(loan_ptr<InputDevice> device, int32_t id, std::string_view name, EButton button) : InputBinary(device, id, name, button) {}

            // Inherited via InputBinary
            virtual bool State() const override;
        };

    public:
        SDL_JoystickGUID Guid;
        SDL_Joystick *Joystick;
        SDL_JoystickID JoystickId;

    protected: // array pointers for hardware specific inputs

        ButtonJoystick *mJoystickButtons;
        AxisJoystick *mJoystickAxes;
        ButtonKeyboard *mKeyboardButtons;
        ButtonMouse *mMouseButtons;

    protected:
        std::string mName;
        EType mType;

        std::vector<AxisPtr> mAxes;
        std::vector<ButtonPtr> mButtons;

    public:
        InputDevice() : Guid(), mName(), mType(EType::Unknown), Joystick(), JoystickId(-1), mJoystickButtons(), mJoystickAxes(), mKeyboardButtons(), mMouseButtons(), mAxes(), mButtons() {}
        InputDevice(const InputDevice &other) = delete;
        InputDevice(const InputDevice &&other) = delete;
        InputDevice &operator=(const InputDevice &other) = delete;

        const std::vector<AxisPtr> &Axes() const { return mAxes; }
        const std::vector<ButtonPtr> &Buttons() const { return mButtons; }

        const std::string &Name() const { return mName; }
        EType Type() const { return mType; }

        /// @brief Finds a button object based on the corresponding enum value.
        /// InputBinary objects are valid until the input device is deleted, so storing the object for later use is a good idea.
        /// @return nullptr if not found, a valid button object
        const ButtonPtr FindButton(EButton button) const;

        std::string BuildDebugPrint() const;

        static InputDevice::loanptr InitKeyboard(std::vector<InputDevice::uniqueptr> &out);
        static InputDevice::loanptr InitMouse(std::vector<InputDevice::uniqueptr> &out);
        static InputDevice::loanptr InitJoystick(std::vector<InputDevice::uniqueptr> &out, SDL_Joystick *joystick);

        virtual ~InputDevice();
    };

}