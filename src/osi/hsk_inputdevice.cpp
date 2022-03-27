#include "hsk_inputdevice.hpp"
#include "hsk_osi.hpp"
#include <sstream>
#include <nameof/nameof.hpp>

namespace hsk
{
    std::vector<EButton> DefinedButtons = std::vector<EButton>();

    const std::vector<EButton> &GetDefinedButtons()
    {
        if (DefinedButtons.size() == 0)
        {

            /*
              Why is this function so ugly? There is no good alternative for an enum this large:
                   * Enum alternatives like https://github.com/aantron/better-enums have a limit to
                     ~ 64 enum entries, due to maximum compiler macro input vector length.
                   * EButton is based on SDL_Scancode which in turn is based on USB standard
                     https://www.usb.org/sites/default/files/documents/hut1_12v2.pdf. Therefore the values
                     should stay as is, which means holes are present

            */

            DefinedButtons.push_back(EButton::Undefined);
            for (int i = 4; i <= 129; i++)
            {
                DefinedButtons.push_back(static_cast<EButton>(i));
            }
            for (int i = 133; i <= 164; i++)
            {
                DefinedButtons.push_back(static_cast<EButton>(i));
            }
            for (int i = 176; i <= 221; i++)
            {
                DefinedButtons.push_back(static_cast<EButton>(i));
            }
            for (int i = 224; i <= 231; i++)
            {
                DefinedButtons.push_back(static_cast<EButton>(i));
            }
            for (int i = 257; i <= 277; i++)
            {
                DefinedButtons.push_back(static_cast<EButton>(i));
            }
            for (int i = 281; i <= 342; i++)
            {
                DefinedButtons.push_back(static_cast<EButton>(i));
            }
        }
        return DefinedButtons;
    }

    const InputDevice::ButtonPtr InputDevice::FindButton(EButton button) const
    {
        for (ButtonPtr buttonptr : mButtons)
        {
            if (buttonptr->Button == button)
            {
                return buttonptr;
            }
        }
        return nullptr;
    }
    std::string InputDevice::BuildDebugPrint() const
    {
        std::stringstream builder;

        builder << Name() << "\n";
        if (mButtons.size() > 0)
        {
            builder << "\t" << mButtons.size() << " Buttons:\n";
            for (ButtonPtr button : mButtons)
            {
                builder << "\t\t" << button->Name << "\n";
            }
        }
        if (mAxes.size() > 0)
        {
            builder << "\t" << mAxes.size() << " Axes:\n";
            for (AxisPtr axis : mAxes)
            {
                builder << "\t\t" << axis->Name << "\n";
            }
        }

        return builder.str();
    }

    InputDevice::loanptr InputDevice::InitKeyboard(std::vector<InputDevice::ptr> &out)
    {
        InputDevice::ptr &device = out.emplace_back(std::make_unique<InputDevice>());
        InputDevice::loanptr result(device);
        result->mName = "Default Keyboard";
        result->mType = EType::Keyboard;
        result->mKeyboardButtons = new ButtonKeyboard[240];

        int index = 0;
        const std::vector<EButton> &buttonList = GetDefinedButtons();

        for (EButton buttonId : buttonList)
        {
            if (buttonId >= EButton::Mouse_Left)
            {
                break;
            }
            ButtonKeyboard button;
            button.Id = static_cast<int32_t>(buttonId);
            button.Button = buttonId;
            button.Name = NAMEOF_ENUM(buttonId);
            result->mKeyboardButtons[index] = button;
            result->mButtons.push_back(result->mKeyboardButtons + index);
            index++;
        }
        return result;
    }
    InputDevice::loanptr InputDevice::InitMouse(std::vector<InputDevice::ptr> &out)
    {
        InputDevice::ptr &device = out.emplace_back(std::make_unique<InputDevice>());
        InputDevice::loanptr result(device);
        result->mName = "Default Mouse";
        result->mType = EType::Mouse;
        result->mMouseButtons = new ButtonMouse[5];

        int index = 0;
        EButton mouseButtons[] = {EButton::Mouse_Left, EButton::Mouse_Right, EButton::Mouse_Middle, EButton::Mouse_X1, EButton::Mouse_X2};

        for (EButton buttonId : mouseButtons)
        {
            ButtonMouse button;
            button.Id = static_cast<int32_t>(buttonId);
            button.Button = buttonId;
            button.Name = NAMEOF_ENUM(buttonId);
            result->mMouseButtons[index] = button;
            result->mButtons.push_back(result->mMouseButtons + index);
            index++;
        }
        return result;
    }
    InputDevice::loanptr InputDevice::InitJoystick(std::vector<InputDevice::ptr> &out, SDL_Joystick *joystick)
    {
        InputDevice::ptr &device = out.emplace_back(std::make_unique<InputDevice>());
        InputDevice::loanptr result(device);
        result->mType = EType::Joystick;
        result->mName = SDL_JoystickName(joystick);
        result->mJoystick = joystick;
        result->mJoystickID = SDL_JoystickInstanceID(joystick);
        result->mId = SDL_JoystickGetGUID(joystick);

        int numAxes = SDL_JoystickNumAxes(joystick);
        int numBalls = SDL_JoystickNumBalls(joystick);
        int numButtons = SDL_JoystickNumButtons(joystick);
        int numHats = SDL_JoystickNumHats(joystick);

        result->mJoystickAxes = new AxisJoystick[numAxes];
        result->mJoystickButtons = new ButtonJoystick[numButtons];

        for (int i = 0; i < numAxes; i++)
        {
            AxisJoystick axis;

            axis.Id = i;
            axis.Axis = (EAxis)i;
            axis.Joystick = joystick;
            axis.Name = NAMEOF_ENUM(axis.Axis);
            result->mJoystickAxes[i] = axis;
            result->mAxes.push_back(result->mJoystickAxes + i);
        }

        for (int i = 0; i < numButtons; i++)
        {
            ButtonJoystick button;
            EButton buttonId;
            if (i < 50)
            {
                buttonId = static_cast<EButton>(static_cast<int32_t>(EButton::JoystickButton_0) + i);
            }

            button.Id = i;
            button.Joystick = joystick;
            button.Button = buttonId;
            button.Name = NAMEOF_ENUM(buttonId);
            result->mJoystickButtons[i] = button;
            result->mButtons.push_back(result->mJoystickButtons + i);
        }

        return result;
    }

    InputDevice::~InputDevice()
    {
        if (mJoystick)
        {
            SDL_JoystickClose(mJoystick);
        }
        delete[] mJoystickAxes;
        delete[] mJoystickButtons;
        delete[] mKeyboardButtons;
        delete[] mMouseButtons;
    }
    int16_t InputDevice::AxisJoystick::State() const { return SDL_JoystickGetAxis(Joystick, Id); }
    bool InputDevice::ButtonJoystick::State() const { return SDL_JoystickGetButton(Joystick, Id) > 0; }
    bool InputDevice::ButtonMouse::State() const
    {
        uint32_t mouseState = SDL_GetMouseState(nullptr, nullptr);
        uint32_t mask;
        switch (Button)
        {
        case EButton::Mouse_Left:
            mask = SDL_BUTTON_LMASK;
            break;
        case EButton::Mouse_Right:
            mask = SDL_BUTTON_RMASK;
            break;
        case EButton::Mouse_Middle:
            mask = SDL_BUTTON_MMASK;
            break;
        case EButton::Mouse_X1:
            mask = SDL_BUTTON_X1MASK;
            break;
        case EButton::Mouse_X2:
            mask = SDL_BUTTON_X2MASK;
            break;
        default:
            mask = 0;
            break;
        }
        return (mouseState & mask) != 0;
    }
    bool InputDevice::ButtonKeyboard::State() const
    {
        int num = 0;
        const uint8_t *arr = SDL_GetKeyboardState(&num);
        if (arr == nullptr || (int)Button >= num)
        {
            return false; // Break out if arr is null or the button encoded in this button object is not a valid keyboard key
        }
        return arr[(int)Button] != 0;
    }
}