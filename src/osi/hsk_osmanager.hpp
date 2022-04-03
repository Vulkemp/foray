#pragma once
#include "../hsk_memory.hpp"
#include "hsk_osi_declares.hpp"
#include <sdl2/SDL.h>
#include <vector>

namespace hsk {
    /// @brief Class of which a single instance needs to be present for interaction with the operating system via SDL
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
        OsManager(const OsManager& other)  = delete;
        OsManager(const OsManager&& other) = delete;
        OsManager& operator=(const OsManager& other) = delete;
        virtual ~OsManager();

        inline static loan_ptr<OsManager> Instance = nullptr;

        /// @brief A collection of all non-standard input devices recognized by this application
        std::vector<loan_ptr<InputDevice>> InputDevices();

      public:
        /// @brief Inits the SDL subsystem and catalogues input devices
        void Init();
        /// @brief Cleans the SDL subsystem
        void Cleanup();

        /// @brief Polls next event from system event queue. Retuns nullptr if no event present
        virtual std::shared_ptr<Event> PollEvent();

      protected:
        /// @brief Translates SDL event structures to Event class objects
        virtual bool TranslateSDLEvent(const SDL_Event& sdl_event, std::shared_ptr<Event>& ref_event);

        virtual std::shared_ptr<Event> TranslateEvent_MouseButton(const SDL_Event& sdl_event);
        virtual std::shared_ptr<Event> TranslateEvent_Keyboard(const SDL_Event& sdl_event);
        virtual std::shared_ptr<Event> TranslateEvent_MouseMoved(const SDL_Event& sdl_event);
        virtual std::shared_ptr<Event> TranslateEvent_JoyAxis(const SDL_Event& sdl_event);
        virtual std::shared_ptr<Event> TranslateEvent_JoyButton(const SDL_JoyButtonEvent& sdl_event);
        virtual std::shared_ptr<Event> TranslateEvent_JoyDevice(const SDL_JoyDeviceEvent& sdl_event);
        virtual std::shared_ptr<Event> TranslateEvent_WindowClosed(const loan_ptr<Window> window, uint32_t timestamp);
        virtual std::shared_ptr<Event> TranslateEvent_WindowResized(const loan_ptr<Window> window, const SDL_WindowEvent& wevent);
        virtual std::shared_ptr<Event> TranslateEvent_WindowFocus(const loan_ptr<Window> window, const SDL_WindowEvent& wevent, bool mouseonly, bool focus);
    };
}  // namespace hsk