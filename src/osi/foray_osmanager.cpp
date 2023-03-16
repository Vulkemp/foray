#include "foray_osmanager.hpp"

#include "../foray_exception.hpp"
#include "foray_osi.hpp"
#include <nameof/nameof.hpp>
#include <sdl2/SDL.h>

namespace foray::osi {
#pragma region lifetime

    OsManager::OsManager() : mMouse(), mKeyboard(), mInputDevices()
    {
        if(sInstance != nullptr)
        {
            Exception::Throw("OsManager manages static SDL state. For this reason only one object of its type may exist!");
        }
        sInstance = this;
    }

    OsManager::~OsManager()
    {
        sInstance = nullptr;
    }

    void OsManager::Init()
    {
        SDL_Init(SDL_INIT_JOYSTICK);
        mKeyboard = InputDevice::InitKeyboard(mInputDevices);
        mMouse    = InputDevice::InitMouse(mInputDevices);

        int numJoySticks = SDL_NumJoysticks();
        for(int i = 0; i < numJoySticks; i++)
        {
            SDL_Joystick* joy = SDL_JoystickOpen(i);
            if(joy)
            {
                InputDevice::InitJoystick(mInputDevices, joy);
            }
        }
    }

    void OsManager::Destroy()
    {
        mInputDevices.clear();
        mLastEvent = nullptr;
        SDL_Quit();
    }

#pragma endregion
#pragma region misc

    std::vector<InputDevice*> OsManager::InputDevices()
    {
        std::vector<InputDevice*> out(mInputDevices.size());
        for(auto& inputdevice : mInputDevices)
        {
            out.push_back(inputdevice.get());
        }
        return out;
    }

    OsManager::EventPollResult OsManager::PollEvent()
    {
        EventPollResult result;
        mLastEvent = nullptr;
        if(SDL_PollEvent(&result.Raw.Data) != 0)
        {
            TranslateSDLEvent(result.Raw.Data);
            result.Cast = mLastEvent ? mLastEvent.get() : nullptr;
            result.Any = true;
        }
        return result;
    }

#pragma endregion
#pragma region event translation

    /// @brief Helper for getting a window id from a generic SDL event struct and matching it to the correct window
    template <typename TEvStr>
    Window* GetWindowPtr(const TEvStr& evstr)
    {
        if(evstr.windowID)
        {
            return Window::FindBySDLId(evstr.windowID);
        }
        return nullptr;
    }

    bool OsManager::TranslateSDLEvent(const SDL_Event& sdl_event)
    {
        switch(sdl_event.type)
        {
            case SDL_DROPTEXT:  // TODO Low Priority Handle Droptext/file events
            case SDL_DROPFILE:  // WindowPtr Drop Event
                break;
            case SDL_KEYUP:
            case SDL_KEYDOWN:
                // Binary Event
                TranslateEvent_Keyboard(sdl_event);
                break;
            case SDL_JOYAXISMOTION:
                // Axis Event
                TranslateEvent_JoyAxis(sdl_event);
                break;
            case SDL_JOYBALLMOTION:
            case SDL_JOYHATMOTION:
                // TODO Low Priority: Handle Joystick Ball & Hat input as directional
                break;
            case SDL_JOYBUTTONUP:
            case SDL_JOYBUTTONDOWN:
                // Binary Event
                TranslateEvent_JoyButton(sdl_event.jbutton);
                break;
            case SDL_JOYDEVICEADDED:
            case SDL_JOYDEVICEREMOVED:
                TranslateEvent_JoyDevice(sdl_event.jdevice);
                break;
            case SDL_MOUSEMOTION:
                // Mouse Move Event
                TranslateEvent_MouseMoved(sdl_event);
                break;
            case SDL_MOUSEBUTTONUP:
            case SDL_MOUSEBUTTONDOWN:
                // Binary Event
                TranslateEvent_MouseButton(sdl_event);
                break;
            case SDL_MOUSEWHEEL:
                TranslateEvent_MouseScroll(sdl_event);
                break;
            case SDL_WINDOWEVENT:
                // Multiple WindowPtr Events
                {
                    SDL_WindowEvent wevent = sdl_event.window;
                    Window*         window = Window::FindBySDLId(wevent.windowID);
                    switch(wevent.event)
                    {
                        case SDL_WINDOWEVENT_CLOSE:
                            TranslateEvent_WindowClosed(window, wevent.timestamp);
                            break;
                        case SDL_WINDOWEVENT_SIZE_CHANGED:
                            TranslateEvent_WindowResized(window, wevent);
                            break;
                        case SDL_WINDOWEVENT_RESTORED:
                            break;
                        case SDL_WINDOWEVENT_ENTER:
                            TranslateEvent_WindowFocus(window, wevent, true, true);
                            break;
                        case SDL_WINDOWEVENT_LEAVE:
                            TranslateEvent_WindowFocus(window, wevent, true, false);
                            break;
                        case SDL_WINDOWEVENT_FOCUS_GAINED:
                            TranslateEvent_WindowFocus(window, wevent, false, true);
                            break;
                        case SDL_WINDOWEVENT_FOCUS_LOST:
                            TranslateEvent_WindowFocus(window, wevent, false, false);
                            break;
                        case SDL_WINDOWEVENT_MINIMIZED:
                        case SDL_WINDOWEVENT_MAXIMIZED:
                        case SDL_WINDOWEVENT_RESIZED:  // Ignored since size_changed is called either way
                        case SDL_WINDOWEVENT_SHOWN:
                        case SDL_WINDOWEVENT_HIDDEN:
                        case SDL_WINDOWEVENT_EXPOSED:
                        case SDL_WINDOWEVENT_MOVED:
                        case SDL_WINDOWEVENT_TAKE_FOCUS:
                        case SDL_WINDOWEVENT_HIT_TEST:
                            break;
                    }
                    break;
                }
            case SDL_CONTROLLERAXISMOTION:  // the SDL controller subsystem is not initialized, controllers are treated like any joystick
            case SDL_CONTROLLERBUTTONUP:
            case SDL_CONTROLLERBUTTONDOWN:
            case SDL_QUIT:  // Quit Event (Handled explicitly via window events at a higher level)
            default:
                break;
        }

        if(!!mLastEvent)
        {
            if(!!mLastEvent->Source)
            {
                Window* window = Window::FindBySDLId(mLastEvent->Source->SDLId());
                window->HandleEvent(mLastEvent.get());
            }
            return true;
        }
        return false;
    }

#pragma region input
    void OsManager::TranslateEvent_MouseButton(const SDL_Event& sdl_event)
    {
        SDL_MouseButtonEvent mbevent = sdl_event.button;
        Window*              window  = GetWindowPtr<SDL_MouseButtonEvent>(mbevent);
        EButton              button  = EButton::Undefined;
        switch(mbevent.button)
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
        const InputBinary* input = mMouse->FindButton(button);
        if(!input)
        {
            FORAY_THROWFMT("unable to find button {} from event on mouse!", NAMEOF_ENUM(button))
        }
        mLastEvent = std::make_unique<EventInputBinary>(window, mbevent.timestamp, mMouse, input, mbevent.state == SDL_PRESSED);
    }

    void OsManager::TranslateEvent_Keyboard(const SDL_Event& sdl_event)
    {
        SDL_KeyboardEvent kbevent = sdl_event.key;
        Window*           window  = GetWindowPtr<SDL_KeyboardEvent>(kbevent);
        EButton           button  = (EButton)(int)kbevent.keysym.scancode;
        if(kbevent.repeat > 0)
        {
            return;
        }
        const InputBinary* input = mKeyboard->FindButton(button);
        if(!input)
        {
            FORAY_THROWFMT("unable to find button {} from event on keyboard!", NAMEOF_ENUM(button))
        }
        mLastEvent = std::make_unique<EventInputBinary>(window, kbevent.timestamp, mKeyboard, input, kbevent.state == SDL_PRESSED);
    }

    void OsManager::TranslateEvent_MouseMoved(const SDL_Event& sdl_event)
    {
        SDL_MouseMotionEvent mevent    = sdl_event.motion;
        Window*              window    = GetWindowPtr<SDL_MouseMotionEvent>(mevent);
        fp32_t               currentx  = (fp32_t)mevent.x;
        fp32_t               currenty  = (fp32_t)mevent.y;
        fp32_t               relativeX = (fp32_t)mevent.xrel;
        fp32_t               relativeY = (fp32_t)mevent.yrel;

        mLastEvent = std::make_unique<EventInputMouseMoved>(window, mevent.timestamp, mMouse, currentx, currenty, relativeX, relativeY);
    }

    void OsManager::TranslateEvent_MouseScroll(const SDL_Event& sdl_event)
    {
        SDL_MouseWheelEvent mevent  = sdl_event.wheel;
        Window*             window  = GetWindowPtr<SDL_MouseWheelEvent>(mevent);
        int32_t             offsetx = mevent.x;
        int32_t             offsety = mevent.y;

        mLastEvent = std::make_unique<EventInputDirectional>(window, mevent.timestamp, mMouse, mMouse->Directionals().front(), offsetx, offsety);
    }

    void OsManager::TranslateEvent_JoyAxis(const SDL_Event& sdl_event)
    {
        SDL_JoyAxisEvent     ev           = sdl_event.jaxis;
        Window*              window       = nullptr;
        InputDevice*         sourceDevice = nullptr;
        const InputAnalogue* input        = nullptr;

        for(std::unique_ptr<InputDevice>& inpDevPtr : mInputDevices)
        {
            if(inpDevPtr->JoystickId() == ev.which)
            {
                sourceDevice = inpDevPtr.get();
                break;
            }
        }
        if(sourceDevice)
        {
            const std::vector<const InputAnalogue*>& axes = sourceDevice->Axes();
            input                                         = axes[ev.axis];
        }
        if(!input)
        {
            Exception::Throw("unable to find a device from event!");
        }
        mLastEvent = std::make_unique<EventInputAnalogue>(window, ev.timestamp, sourceDevice, input, ev.value);
    }

    void OsManager::TranslateEvent_JoyButton(const SDL_JoyButtonEvent& sdl_event)
    {
        Window*            window       = nullptr;
        InputDevice*       sourceDevice = nullptr;
        const InputBinary* input        = nullptr;
        EButton            button;

        for(std::unique_ptr<InputDevice>& inpDevPtr : mInputDevices)
        {
            if(inpDevPtr->JoystickId() == sdl_event.which)
            {
                sourceDevice = inpDevPtr.get();
                break;
            }
        }
        if(sdl_event.button <= 50 && sourceDevice)
        {
            button = static_cast<EButton>(static_cast<int>(EButton::JoystickButton_0) + static_cast<int>(sdl_event.button));
            input  = sourceDevice->FindButton(button);
        }
        if(!input)
        {
            FORAY_THROWFMT("unable to find button {} from event on a device!", NAMEOF_ENUM(button))
        }
        mLastEvent = std::make_unique<EventInputBinary>(window, sdl_event.timestamp, sourceDevice, input, sdl_event.state == SDL_PRESSED);
    }

    void OsManager::TranslateEvent_JoyDevice(const SDL_JoyDeviceEvent& sdl_event)
    {

        InputDevice* sourceDevice;
        for(std::unique_ptr<InputDevice>& inpDevPtr : mInputDevices)
        {
            if(inpDevPtr->JoystickId() == sdl_event.which)
            {
                sourceDevice = inpDevPtr.get();
                break;
            }
        }
        // TODO edit mInputDevices and query new data (low priority)
        mLastEvent = std::make_unique<EventInputDeviceAvailability>(sdl_event.timestamp, sourceDevice, sdl_event.which == SDL_JOYDEVICEADDED);
    }

#pragma endregion
#pragma region window

    void OsManager::TranslateEvent_WindowClosed(Window* window, uint32_t timestamp)
    {
        mLastEvent = std::make_unique<EventWindowCloseRequested>(window, timestamp);
    }

    void OsManager::TranslateEvent_WindowFocus(Window* window, const SDL_WindowEvent& wevent, bool mouseonly, bool focus)
    {
        mLastEvent = std::make_unique<EventWindowFocusChanged>(window, wevent.timestamp, focus, focus && !mouseonly);
    }
    void OsManager::TranslateEvent_WindowResized(Window* window, const SDL_WindowEvent& wevent)
    {
        VkExtent2D newSize{(uint32_t)wevent.data1, (uint32_t)wevent.data2};
        mLastEvent = std::make_unique<EventWindowResized>(window, wevent.timestamp, newSize);
    }

#pragma endregion
#pragma endregion
}  // namespace foray::osi