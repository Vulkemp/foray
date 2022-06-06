#include "hsk_osmanager.hpp"
#include "hsk_osi.hpp"
#include <nameof/nameof.hpp>
#include <sdl2/SDL.h>

namespace hsk {
#pragma region lifetime

    OsManager::OsManager() : mInputDevices(), mMouse(), mKeyboard()
    {
        if(sInstance != nullptr)
        {
            Exception::Throw("OsManager manages static SDL state. For this reason only one object of its type may exist!");
        }
        sInstance = this;
    }

    OsManager::~OsManager() { sInstance = nullptr; }

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

    void OsManager::Cleanup()
    {
        mInputDevices.clear();
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

    Event::ptr OsManager::PollEvent()
    {
        Event::ptr out;
        SDL_Event  rawevent;
        if(SDL_PollEvent(&rawevent) != 0)
        {
            TranslateSDLEvent(rawevent, out);
        }
        return out;
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

    bool OsManager::TranslateSDLEvent(const SDL_Event& sdl_event, Event::ptr& result)
    {
        result = nullptr;
        switch(sdl_event.type)
        {
            case SDL_DROPTEXT:  // TODO Low Priority Handle Droptext/file events
            case SDL_DROPFILE:  // WindowPtr Drop Event
                break;
            case SDL_KEYUP:
            case SDL_KEYDOWN:
                // Binary Event
                result = TranslateEvent_Keyboard(sdl_event);
                break;
            case SDL_JOYAXISMOTION:
                // Axis Event
                result = TranslateEvent_JoyAxis(sdl_event);
                break;
            case SDL_JOYBALLMOTION:  // TODO Low Priority: Handle Joystick Ball & Hat input
            case SDL_JOYHATMOTION:
                // Neither true Analogue nor Binary -> Requires additional event structure
                break;
            case SDL_JOYBUTTONUP:
            case SDL_JOYBUTTONDOWN:
                // Binary Event
                result = TranslateEvent_JoyButton(sdl_event.jbutton);
                break;
            case SDL_JOYDEVICEADDED:
            case SDL_JOYDEVICEREMOVED:
                result = TranslateEvent_JoyDevice(sdl_event.jdevice);
                break;
            case SDL_MOUSEMOTION:
                // Mouse Move Event
                result = TranslateEvent_MouseMoved(sdl_event);
                break;
            case SDL_MOUSEBUTTONUP:
            case SDL_MOUSEBUTTONDOWN:
                // Binary Event
                result = TranslateEvent_MouseButton(sdl_event);
                break;
            case SDL_MOUSEWHEEL:  // Todo: Handle Mousewheel
                // Neither true Analogue nor Binary -> Requires additional event structure
                break;
            case SDL_WINDOWEVENT:
                // Multiple WindowPtr Events
                {
                    SDL_WindowEvent wevent = sdl_event.window;
                    Window*         window = Window::FindBySDLId(wevent.windowID);
                    switch(wevent.event)
                    {
                        case SDL_WINDOWEVENT_CLOSE:
                            result = TranslateEvent_WindowClosed(window, wevent.timestamp);
                            break;
                        case SDL_WINDOWEVENT_RESIZED:  // Ignored since size_changed is called either way
                        case SDL_WINDOWEVENT_SHOWN:
                        case SDL_WINDOWEVENT_HIDDEN:
                        case SDL_WINDOWEVENT_EXPOSED:
                        case SDL_WINDOWEVENT_MOVED:
                        case SDL_WINDOWEVENT_SIZE_CHANGED:
                            result = TranslateEvent_WindowResized(window, wevent);
                            break;
                        case SDL_WINDOWEVENT_MINIMIZED:
                        case SDL_WINDOWEVENT_MAXIMIZED:
                        case SDL_WINDOWEVENT_RESTORED:
                        case SDL_WINDOWEVENT_ENTER:
                            result = TranslateEvent_WindowFocus(window, wevent, true, true);
                            break;
                        case SDL_WINDOWEVENT_LEAVE:
                            result = TranslateEvent_WindowFocus(window, wevent, true, false);
                            break;
                        case SDL_WINDOWEVENT_FOCUS_GAINED:
                            result = TranslateEvent_WindowFocus(window, wevent, false, true);
                            break;
                        case SDL_WINDOWEVENT_FOCUS_LOST:
                            result = TranslateEvent_WindowFocus(window, wevent, false, false);
                            break;
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

        if(result)
        {
            if(result->Source() != nullptr)
            {
                Window* window = Window::FindBySDLId(result->Source()->SDLId());
                window->HandleEvent(result);
            }
            return true;
        }
        return false;
    }

#pragma region input
    Event::ptr OsManager::TranslateEvent_MouseButton(const SDL_Event& sdl_event)
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
            HSK_THROWFMT("unable to find button {} from event on mouse!", NAMEOF_ENUM(button))
        }
        std::shared_ptr<EventInputBinary> result = std::make_shared<EventInputBinary>(window, mbevent.timestamp, mMouse, input, mbevent.state == SDL_PRESSED);
        return result;
    }

    Event::ptr OsManager::TranslateEvent_Keyboard(const SDL_Event& sdl_event)
    {
        SDL_KeyboardEvent kbevent = sdl_event.key;
        Window*           window  = GetWindowPtr<SDL_KeyboardEvent>(kbevent);
        EButton           button  = (EButton)(int)kbevent.keysym.scancode;
        if(kbevent.repeat > 0)
        {
            return nullptr;
        }
        const InputBinary* input = mKeyboard->FindButton(button);
        if(!input)
        {
            HSK_THROWFMT("unable to find button {} from event on keyboard!", NAMEOF_ENUM(button))
        }
        std::shared_ptr<EventInputBinary> result = std::make_shared<EventInputBinary>(window, kbevent.timestamp, mKeyboard, input, kbevent.state == SDL_PRESSED);
        return result;
    }

    Event::ptr OsManager::TranslateEvent_MouseMoved(const SDL_Event& sdl_event)
    {
        SDL_MouseMotionEvent mevent    = sdl_event.motion;
        Window*              window    = GetWindowPtr<SDL_MouseMotionEvent>(mevent);
        fp32_t               currentx  = mevent.x;
        fp32_t               currenty  = mevent.y;
        fp32_t               relativeX = mevent.xrel;
        fp32_t               relativeY = mevent.yrel;

        std::shared_ptr<EventInputMouseMoved> result = std::make_shared<EventInputMouseMoved>(window, mevent.timestamp, nullptr, currentx, currenty, relativeX, relativeY);
        return result;
    }

    Event::ptr OsManager::TranslateEvent_JoyAxis(const SDL_Event& sdl_event)
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
        return std::make_shared<EventInputAnalogue>(window, ev.timestamp, sourceDevice, input, ev.value);
    }

    Event::ptr OsManager::TranslateEvent_JoyButton(const SDL_JoyButtonEvent& sdl_event)
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
            HSK_THROWFMT("unable to find button {} from event on a device!", NAMEOF_ENUM(button))
        }
        return std::make_shared<EventInputBinary>(window, sdl_event.timestamp, sourceDevice, input, sdl_event.state == SDL_PRESSED);
    }

    Event::ptr OsManager::TranslateEvent_JoyDevice(const SDL_JoyDeviceEvent& sdl_event)
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
        return std::make_shared<EventInputDeviceAvailability>(sdl_event.timestamp, sourceDevice, sdl_event.which == SDL_JOYDEVICEADDED);
    }

#pragma endregion
#pragma region window

    Event::ptr OsManager::TranslateEvent_WindowClosed(Window* window, uint32_t timestamp) { return std::make_shared<EventWindowCloseRequested>(window, timestamp); }

    Event::ptr OsManager::TranslateEvent_WindowFocus(Window* window, const SDL_WindowEvent& wevent, bool mouseonly, bool focus)
    {
        return std::make_shared<EventWindowFocusChanged>(window, wevent.timestamp, focus, focus && !mouseonly);
    }
    Event::ptr OsManager::TranslateEvent_WindowResized(Window* window, const SDL_WindowEvent& wevent)
    {
        Extent2D newSize{wevent.data1, wevent.data2};
        return std::make_shared<EventWindowResized>(window, wevent.timestamp, newSize);
    }

#pragma endregion
#pragma endregion
}  // namespace hsk