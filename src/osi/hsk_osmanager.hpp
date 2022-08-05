#pragma once
#include "../hsk_basics.hpp"
#include "hsk_osi_declares.hpp"
#include <sdl2/SDL.h>
#include <vector>

namespace hsk {
    /// @brief Class of which a single instance needs to be present for interaction with the operating system via SDL
    class OsManager
    {
      public:
        inline InputDevice*       Mouse() { return mMouse; }
        inline const InputDevice* Mouse() const { return mMouse; }
        inline InputDevice*       Keyboard() { return mKeyboard; }
        inline const InputDevice* Keyboard() const { return mKeyboard; }
        inline static OsManager*  Instance() { return sInstance; }

        OsManager();

        OsManager(const OsManager& other)            = delete;
        OsManager(const OsManager&& other)           = delete;
        OsManager& operator=(const OsManager& other) = delete;
        virtual ~OsManager();


        /// @brief A collection of all non-standard input devices recognized by this application
        virtual std::vector<InputDevice*> InputDevices();

        /// @brief Inits the SDL subsystem and catalogues input devices
        virtual void Init();
        /// @brief Cleans the SDL subsystem
        virtual void Cleanup();

        /// @brief Polls next event from system event queue. Retuns nullptr if no event present
        /// @remark The pointer returned is valid until the next time PollEvent() is invoked.
        virtual const Event* PollEvent();

      protected:
        /// @brief Mouse input device. Assumed standard and always present
        InputDevice* mMouse;
        /// @brief Keyboard input device. Assumed standard and always present
        InputDevice* mKeyboard;

        inline static OsManager* sInstance = nullptr;

        /// @brief A collection of all non-standard input devices recognized by this application
        std::vector<std::unique_ptr<InputDevice>> mInputDevices;

        /// @brief Translates SDL event structures to Event class objects
        virtual bool TranslateSDLEvent(const SDL_Event& sdl_event, Event*& out_event);

        virtual Event* TranslateEvent_MouseButton(const SDL_Event& sdl_event);
        virtual Event* TranslateEvent_Keyboard(const SDL_Event& sdl_event);
        virtual Event* TranslateEvent_MouseMoved(const SDL_Event& sdl_event);
        virtual Event* TranslateEvent_MouseScroll(const SDL_Event& sdl_event);
        virtual Event* TranslateEvent_JoyAxis(const SDL_Event& sdl_event);
        virtual Event* TranslateEvent_JoyButton(const SDL_JoyButtonEvent& sdl_event);
        virtual Event* TranslateEvent_JoyDevice(const SDL_JoyDeviceEvent& sdl_event);
        virtual Event* TranslateEvent_WindowClosed(Window* window, uint32_t timestamp);
        virtual Event* TranslateEvent_WindowResized(Window* window, const SDL_WindowEvent& wevent);
        virtual Event* TranslateEvent_WindowFocus(Window* window, const SDL_WindowEvent& wevent, bool mouseonly, bool focus);

        Event* mLastEvent = nullptr;
    };
}  // namespace hsk