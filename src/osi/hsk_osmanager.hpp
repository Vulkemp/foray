#pragma once
#include "hsk_osi_declares.hpp"
#include "../hsk_memory.hpp"
#include <sdl2/SDL.h>
#include <vector>

namespace hsk
{
    class OsManager
    {
    protected:
        /// @brief A collection of all non-standard input devices recognized by this application
        std::vector<std::unique_ptr<InputDevice>> mInputDevices;

    public:
        /// @brief Mouse input device. Assumed standard and always present
        loan_ptr<InputDevice> Mouse;
        /// @brief Keyboard input device. Assumed standard and always present
        loan_ptr<InputDevice> Keyboard;

        OsManager();

    public:
        OsManager(const OsManager &other) = delete;
        OsManager(const OsManager &&other) = delete;
        OsManager &operator=(const OsManager &other) = delete;
        virtual ~OsManager();

        inline static OsManager* Instance = nullptr;
        
        /// @brief A collection of all non-standard input devices recognized by this application
        std::vector<loan_ptr<InputDevice>> InputDevices();

    public:
        void Init();
        void Cleanup();

        std::shared_ptr<Event> PollEvent();

    protected:
        bool HandleSDLEvent(const SDL_Event &sdl_event, std::shared_ptr<Event> &ref_event);

        std::shared_ptr<Event> TranslateEvent_MouseButton(const SDL_Event &sdl_event);
        std::shared_ptr<Event> TranslateEvent_Keyboard(const SDL_Event &sdl_event);
        std::shared_ptr<Event> TranslateEvent_MouseMoved(const SDL_Event &sdl_event);
        std::shared_ptr<Event> TranslateEvent_JoyAxis(const SDL_Event &sdl_event);
        std::shared_ptr<Event> TranslateEvent_JoyButton(const SDL_JoyButtonEvent &sdl_event);
        std::shared_ptr<Event> TranslateEvent_WindowClosed(const loan_ptr<Window> window, uint32_t timestamp);
        std::shared_ptr<Event> TranslateEvent_WindowResized(const loan_ptr<Window> window, const SDL_WindowEvent &wevent);
        std::shared_ptr<Event> TranslateEvent_WindowFocus(const loan_ptr<Window> window, const SDL_WindowEvent &wevent, bool mouseonly, bool focus);
    };
}