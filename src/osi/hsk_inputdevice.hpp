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
    class InputDevice
    {
    public:
        using loanptr = loan_ptr<InputDevice>;
        using ptr = std::unique_ptr<InputDevice>;

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

    protected:
        SDL_JoystickGUID mId;
        SDL_Joystick *mJoystick;
        SDL_JoystickID mJoystickID;
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
        InputDevice() : mId(), mName(), mType(EType::Unknown), mJoystick(), mJoystickID(-1), mJoystickButtons(), mJoystickAxes(), mKeyboardButtons(), mMouseButtons(), mAxes(), mButtons() {}
        InputDevice(const InputDevice &other) = delete;
        InputDevice(const InputDevice &&other) = delete;
        InputDevice &operator=(const InputDevice &other) = delete;

        const std::vector<AxisPtr>& Axes() const { return mAxes; }
        const std::vector<ButtonPtr>& Buttons() const { return mButtons; }

        const std::string &Name() const { return mName; }
        EType Type() const { return mType; }

        /// @brief Finds a button object based on the corresponding enum value.
        /// InputBinary objects are valid until the input device is deleted, so storing the object for later use is a good idea.
        /// @return nullptr if not found, a valid button object
        const ButtonPtr FindButton(EButton button) const;

        std::string BuildDebugPrint() const;

        SDL_JoystickID JoystickID() const { return mJoystickID; }

        static InputDevice::loanptr InitKeyboard(std::vector<InputDevice::ptr> &out);
        static InputDevice::loanptr InitMouse(std::vector<InputDevice::ptr> &out);
        static InputDevice::loanptr InitJoystick(std::vector<InputDevice::ptr> &out, SDL_Joystick *joystick);

        virtual ~InputDevice();
    };

}