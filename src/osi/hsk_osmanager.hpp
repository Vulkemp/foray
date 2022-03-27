#include "hsk_event.hpp"
#include "hsk_inputdevice.hpp"
#include <sdl2/SDL.h>
#include "hsk_window.hpp"
#include <vector>

namespace hsk
{
    class OsManager
    {
    protected:
        /// @brief A collection of all non-standard input devices recognized by this application
        std::vector<InputDevice::ptr> m_InputDevices;

    public:
        /// @brief Mouse input device. Assumed standard and always present
        InputDevice::loanptr m_Mouse;
        /// @brief Keyboard input device. Assumed standard and always present
        InputDevice::loanptr m_Keyboard;

        OsManager();

    public:
        OsManager(const OsManager &other) = delete;
        OsManager(const OsManager &&other) = delete;
        OsManager &operator=(const OsManager &other) = delete;
        virtual ~OsManager();

        inline static OsManager* Instance = nullptr;
        
        /// @brief A collection of all non-standard input devices recognized by this application
        std::vector<InputDevice::loanptr> InputDevices();

    public:
        void Init();
        void Cleanup();

        Event::ptr PollEvent();

    protected:
        bool HandleSDLEvent(const SDL_Event &sdl_event, Event::ptr &ref_event);

        Event::ptr TranslateEvent_MouseButton(const SDL_Event &sdl_event);
        Event::ptr TranslateEvent_Keyboard(const SDL_Event &sdl_event);
        Event::ptr TranslateEvent_MouseMoved(const SDL_Event &sdl_event);
        Event::ptr TranslateEvent_JoyAxis(const SDL_Event &sdl_event);
        Event::ptr TranslateEvent_JoyButton(const SDL_JoyButtonEvent &sdl_event);
        Event::ptr TranslateEvent_WindowClosed(const Window::loanptr window, uint32_t timestamp);
        Event::ptr TranslateEvent_WindowResized(const Window::loanptr window, const SDL_WindowEvent &wevent);
    };
}