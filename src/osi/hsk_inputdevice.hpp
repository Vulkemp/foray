#pragma once
#include "hsk_osi_declares.hpp"
#include "hsk_helpers.hpp"
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

        class AxisBase
        {
        public:
            int32_t Id;
            std::string Name;
            EAxis Axis;

        protected:
            AxisBase() : Id(-1), Name() {}
            AxisBase(int32_t id, const std::string &name, EAxis axis) : Id(id), Name(name) {}

        public:
            virtual int16_t State() const = 0;
        };

        class ButtonBase
        {
        public:
            int32_t Id;
            std::string Name;
            EButton Button;

        protected:
            ButtonBase() : Id{}, Name(), Button() {}
            ButtonBase(int32_t id, const std::string &name, EButton button) : Id(id), Name(name), Button(button) {}

        public:
            virtual bool State() const = 0;
        };

        using AxisPtr = AxisBase *;
        using ButtonPtr = ButtonBase *;

        class AxisJoystick : public AxisBase
        {
        public:
            SDL_Joystick *Joystick;

            AxisJoystick() : AxisBase(), Joystick() {}
            AxisJoystick(int32_t id, const std::string &name, SDL_Joystick *joystick) : AxisBase(id, name, (EAxis)id), Joystick(joystick) {}

            // Inherited via Axis
            virtual int16_t State() const override;
        };

        class ButtonJoystick : public ButtonBase
        {
        public:
            SDL_Joystick *Joystick;

            ButtonJoystick() : ButtonBase(), Joystick() {}
            ButtonJoystick(int32_t id, const std::string &name, EButton button, SDL_Joystick *joystick) : ButtonBase(id, name, button), Joystick(joystick) {}

            // Inherited via ButtonBase
            virtual bool State() const override;
        };

        class ButtonMouse : public ButtonBase
        {
        public:
            ButtonMouse() : ButtonBase() {}
            ButtonMouse(int32_t id, const std::string &name, EButton button) : ButtonBase(id, name, button) {}

            // Inherited via ButtonBase
            virtual bool State() const override;
        };

        class ButtonKeyboard : public ButtonBase
        {
        public:
            ButtonKeyboard() : ButtonBase() {}
            ButtonKeyboard(int32_t id, const std::string &name, EButton button) : ButtonBase(id, name, button) {}

            // Inherited via ButtonBase
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
        InputDevice() : mId(), mName(), mType(EType::Unknown), mJoystick(), mJoystickID(), mJoystickButtons(), mJoystickAxes(), mKeyboardButtons(), mMouseButtons(), mAxes(), mButtons() {}
        InputDevice(const InputDevice &other) = delete;
        InputDevice(const InputDevice &&other) = delete;
        InputDevice &operator=(const InputDevice &other) = delete;

        const std::vector<AxisPtr> Axes() const { return mAxes; }
        const std::vector<ButtonPtr> Buttons() const { return mButtons; }

        const std::string &Name() const { return mName; }
        EType Type() const { return mType; }

        /// @brief Finds a button object based on the corresponding enum value.
        /// ButtonBase objects are valid until the input device is deleted, so storing the object for later use is a good idea.
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