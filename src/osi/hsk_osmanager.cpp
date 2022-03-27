#include "hsk_osmanager.hpp"
#include "hsk_osi.hpp"
#include <sdl2/SDL.h>

namespace hsk
{
    OsManager::OsManager()
        : m_InputDevices(), m_Mouse(), m_Keyboard()
    {
        if (Instance != nullptr)
        {
            throw std::runtime_error("OsManager manages static SDL state. For this reason only one object of its type may exist!");
        }
        Instance = this;
    }

    OsManager::~OsManager()
    {
        Instance = nullptr;
    }

    void OsManager::Init()
    {
        SDL_Init(SDL_INIT_JOYSTICK);
        m_Keyboard = InputDevice::InitKeyboard(m_InputDevices);
        m_Mouse = InputDevice::InitMouse(m_InputDevices);
        
        int numJoySticks = SDL_NumJoysticks();
        for (int i = 0; i < numJoySticks; i++)
        {
            SDL_Joystick *joy = SDL_JoystickOpen(i);
            if (joy)
            {
                InputDevice::InitJoystick(m_InputDevices, joy);
            }
        }
    }

    void OsManager::Cleanup()
    {
        m_InputDevices.clear();
        SDL_Quit();
    }

    std::vector<InputDevice::loanptr> OsManager::InputDevices()
    {
        std::vector<InputDevice::loanptr> out(m_InputDevices.size());
        for (auto &inputdevice : m_InputDevices)
        {
            out.push_back(inputdevice);
        }
        return out;
    }

    Event::ptr OsManager::PollEvent()
    {
        Event::ptr out;
        SDL_Event rawevent;
        if (SDL_PollEvent(&rawevent) != 0)
        {
            HandleSDLEvent(rawevent, out);
        }
        return out;
    }

    template <typename TEvStr>
    Window::loanptr GetWindowPtr(OsManager *manager, const TEvStr &evstr)
    {
        if (evstr.windowID)
        {
            return Window::FindBySDLId(evstr.windowID);
        }
        return nullptr;
    }

    Event::ptr OsManager::TranslateEvent_MouseButton(const SDL_Event &sdl_event)
    {
        SDL_MouseButtonEvent mbevent = sdl_event.button;
        Window::loanptr window = GetWindowPtr<SDL_MouseButtonEvent>(this, mbevent);
        EButton button = EButton::Undefined;
        switch (mbevent.button)
        {
        case SDL_BUTTON_LEFT:
            button = EButton::Mouse_Left;
            break;
        case SDL_BUTTON_MIDDLE:
            button = EButton::Mouse_Middle;
            break;
        case SDL_BUTTON_RIGHT:
            button = EButton::Mouse_Right;
            break;
        case SDL_BUTTON_X1:
            button = EButton::Mouse_X1;
            break;
        case SDL_BUTTON_X2:
            button = EButton::Mouse_X2;
            break;
        }
        std::shared_ptr<EventInputBinary> result = std::make_shared<EventInputBinary>(window, mbevent.timestamp, m_Mouse, button, mbevent.state == SDL_PRESSED);
        return result;
    }

    Event::ptr OsManager::TranslateEvent_Keyboard(const SDL_Event &sdl_event)
    {
        SDL_KeyboardEvent kbevent = sdl_event.key;
        Window::loanptr window = GetWindowPtr<SDL_KeyboardEvent>(this, kbevent);
        EButton button = (EButton)(int)kbevent.keysym.scancode;
        if (kbevent.repeat > 0)
        {
            return nullptr;
        }
        std::shared_ptr<EventInputBinary> result = std::make_shared<EventInputBinary>(window, kbevent.timestamp, m_Keyboard, button, kbevent.state == SDL_PRESSED);
        return result;
    }

    Event::ptr OsManager::TranslateEvent_MouseMoved(const SDL_Event &sdl_event)
    {
        SDL_MouseMotionEvent mevent = sdl_event.motion;
        Window::loanptr window = GetWindowPtr<SDL_MouseMotionEvent>(this, mevent);
        fp32_t currentx = mevent.x;
        fp32_t currenty = mevent.y;

        std::shared_ptr<EventInputMouseMoved> result = std::make_shared<EventInputMouseMoved>(window, mevent.timestamp, nullptr, currentx, currenty);
        return result;
    }

    Event::ptr OsManager::TranslateEvent_WindowClosed(const Window::loanptr window, uint32_t timestamp) { return std::make_shared<EventWindowCloseRequested>(window, 0); }

    Event::ptr OsManager::TranslateEvent_WindowResized(const Window::loanptr window, const SDL_WindowEvent &wevent)
    {
        return std::make_shared<EventWindowResized>(window, wevent.timestamp, Extent2D{wevent.data1, wevent.data2});
    }

    Event::ptr OsManager::TranslateEvent_JoyAxis(const SDL_Event &sdl_event)
    {
        SDL_JoyAxisEvent ev = sdl_event.jaxis;
        Window::loanptr window = nullptr;
        InputDevice::loanptr sourceDevice = nullptr;
        for (InputDevice::loanptr inpDevPtr : m_InputDevices)
        {
            if (inpDevPtr->JoystickID() == ev.which)
            {
                sourceDevice = inpDevPtr;
            }
        }
        return std::make_shared<EventInputAnalogue>(window, ev.timestamp, sourceDevice, ev.axis, ev.value);
    }

    Event::ptr OsManager::TranslateEvent_JoyButton(const SDL_JoyButtonEvent &sdl_event)
    {
        Window::loanptr window = nullptr;
        InputDevice::loanptr sourceDevice = nullptr;
        for (InputDevice::loanptr inpDevPtr : m_InputDevices)
        {
            if (inpDevPtr->JoystickID() == sdl_event.which)
            {
                sourceDevice = inpDevPtr;
            }
        }
        EButton button;
        if (sdl_event.button >= 50)
        {
            button = EButton::Undefined;
        }
        else
        {
            button = static_cast<EButton>(static_cast<int>(EButton::JoystickButton_0) + static_cast<int>(sdl_event.button));
        }
        return std::make_shared<EventInputBinary>(window, sdl_event.timestamp, sourceDevice, button, sdl_event.state == SDL_PRESSED);
    }

    bool OsManager::HandleSDLEvent(const SDL_Event &sdl_event, Event::ptr &ref_event)
    {
        ref_event = nullptr;
        switch (sdl_event.type)
        {
        case SDL_CONTROLLERAXISMOTION: // the SDL controller subsystem is not initialized, controllers are treated like any joystick
        case SDL_CONTROLLERBUTTONUP:
        case SDL_CONTROLLERBUTTONDOWN:
            return false;
            break;
        case SDL_DROPTEXT: // Todo: Handle Droptext/file events
        case SDL_DROPFILE:
            // WindowPtr Drop Event
            return false;
            break;
        case SDL_KEYUP:
        case SDL_KEYDOWN:
            // Binary Event
            ref_event = TranslateEvent_Keyboard(sdl_event);
            return true;
        case SDL_JOYAXISMOTION:
            // Axis Event
            ref_event = TranslateEvent_JoyAxis(sdl_event);
            return true;
        case SDL_JOYBALLMOTION: // TODO: Handle Joystick Ball & Hat input
        case SDL_JOYHATMOTION:
            return false;
            break;
        case SDL_JOYBUTTONUP:
        case SDL_JOYBUTTONDOWN:
            // Binary Event
            ref_event = TranslateEvent_JoyButton(sdl_event.jbutton);
            return true;
        case SDL_MOUSEMOTION:
            // Mouse Move Event
            ref_event = TranslateEvent_MouseMoved(sdl_event);
            return true;
        case SDL_MOUSEBUTTONUP:
        case SDL_MOUSEBUTTONDOWN:
            // Binary Event
            ref_event = TranslateEvent_MouseButton(sdl_event);
            return true;
        case SDL_MOUSEWHEEL: // Todo: Handle Mousewheel
            // Axis Event
            return false;
        case SDL_QUIT:
            // Quit Event
            return false;
        case SDL_WINDOWEVENT:
            // Multiple WindowPtr Events
            {
                SDL_WindowEvent wevent = sdl_event.window;
                Window::loanptr window = Window::FindBySDLId(wevent.windowID);
                switch (wevent.event)
                {
                case SDL_WINDOWEVENT_CLOSE:
                    ref_event = TranslateEvent_WindowClosed(window, wevent.timestamp);
                    return true;
                case SDL_WINDOWEVENT_RESIZED:
                    ref_event = TranslateEvent_WindowResized(window, wevent);
                    return true;
                case SDL_WINDOWEVENT_SHOWN:
                case SDL_WINDOWEVENT_HIDDEN:
                case SDL_WINDOWEVENT_EXPOSED:
                case SDL_WINDOWEVENT_MOVED:
                case SDL_WINDOWEVENT_SIZE_CHANGED:
                case SDL_WINDOWEVENT_MINIMIZED:
                case SDL_WINDOWEVENT_MAXIMIZED:
                case SDL_WINDOWEVENT_RESTORED:
                case SDL_WINDOWEVENT_ENTER:
                case SDL_WINDOWEVENT_LEAVE:
                case SDL_WINDOWEVENT_FOCUS_GAINED:
                case SDL_WINDOWEVENT_FOCUS_LOST:
                case SDL_WINDOWEVENT_TAKE_FOCUS:
                case SDL_WINDOWEVENT_HIT_TEST:
                default:
                    return false;
                }
            }
        default:
            return false;
        }
    }

}