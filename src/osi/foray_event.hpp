#pragma once
#include "../foray_basics.hpp"
#include "../foray_vulkan.hpp"
#include "foray_helpers.hpp"
#include "foray_input.hpp"
#include "foray_osi_declares.hpp"
#include <memory>
#include <sdl2/SDL.h>

namespace foray::osi {
    /// @brief Event Base class
    class Event : public Polymorphic
    {
      public:
        /// @brief Event type enum, allows handling events in switch structures
        enum class EType : int16_t
        {
            Undefined = -1,
            /// @brief Analogue inputs, ex. controller stick axis
            InputAnalogue,
            /// @brief Binary inputs, ex. buttons, keys
            InputBinary,
            /// @brief Directional stateless inputs, ex. scroll wheel, joystick hatswitches, etc.
            InputDirectional,
            /// @brief Mouse position input type
            InputMouseMoved,
            /// @brief WindowPtr resized
            WindowResized,
            /// @brief WindowPtr lost/gained focus
            WindowFocusChanged,
            /// @brief User requested window closed
            WindowCloseRequested,
            /// @brief File/Folder was dragged on top of the window
            WindowItemDropped,
            /// @brief An input device was connected or removed
            InputDeviceAvailability,
            /// @brief Custom type
            Custom,
            ENUM_MAX
        };

        inline Event() = default;
        inline Event(Window* const source, const uint32_t timestamp, const EType type) : Source(source), Timestamp(timestamp), Type(type), CustomType(0) {}
        inline Event(Window* const source, const uint32_t timestamp, const int8_t customtype) : Source(source), Timestamp(timestamp), Type(EType::Custom), CustomType(customtype) {}
        inline virtual ~Event() {}

        /// @brief Source window that recorded the event, if applicable
        Window* Source = nullptr;
        /// @brief Timestamp when the action was recorded
        uint32_t Timestamp = 0;
        /// @brief Event type
        EType Type = EType::Undefined;
        /// @brief For custom event type overloads, this value may be set
        int16_t CustomType = 0;
        /// @brief Raw SDL_Event data this event was derived from
        SDL_Event RawSdlEventData = {};
    };

    class EventInput : public Event
    {
      public:
        inline EventInput() = default;
        inline EventInput(Window* const source, const uint32_t timestamp, const EType type, InputDevice* const device) : Event(source, timestamp, type), SourceDevice(device) {}

        /// @brief Input Device that the input was read from
        InputDevice* SourceDevice = nullptr;
    };

    class EventInputDeviceAvailability : public EventInput
    {
      public:
        inline EventInputDeviceAvailability(const uint32_t timestamp, InputDevice* const device, const bool added)
            : EventInput(nullptr, timestamp, Event::EType::InputDeviceAvailability, device), Added(added)
        {
        }

        /// @brief True if device was added, false if device was removed
        bool Added = false;
    };

    class EventInputAnalogue : public EventInput
    {
      public:
        inline EventInputAnalogue() = default;
        inline EventInputAnalogue(Window* const source, const uint32_t timestamp, InputDevice* device, const InputAnalogue* axis, int16_t current)
            : EventInput(source, timestamp, EType::InputAnalogue, device), SourceInput(axis), State(current)
        {
        }

        /// @brief axis that was moved
        const InputAnalogue* SourceInput = nullptr;
        /// @brief Current reading from the axis
        int16_t State = 0;
    };

    class EventInputBinary : public EventInput
    {
      public:
        inline EventInputBinary() = default;
        inline EventInputBinary(Window* const source, const uint32_t timestamp, InputDevice* device, const InputBinary* button, bool pressed)
            : EventInput(source, timestamp, EType::InputBinary, device), SourceInput(button), State(pressed)
        {
        }

        /// @brief The button that was pressed or released
        const InputBinary* SourceInput = nullptr;
        /// @brief If true, the button was pressed - released otherwise
        bool State = 0;
    };

    class EventInputDirectional : public EventInput
    {
      public:
        inline EventInputDirectional() = default;
        inline EventInputDirectional(Window* const source, const uint32_t timestamp, InputDevice* device, const InputDirectional* inputsource, int32_t offsetX, int32_t offsetY)
            : EventInput(source, timestamp, EType::InputBinary, device), SourceInput(inputsource), OffsetX(offsetX), OffsetY(offsetY)
        {
        }

        /// @brief The directional input that was triggered
        const InputDirectional* SourceInput = nullptr;
        /// @brief The offset in X direction
        int32_t OffsetX = 0;
        /// @brief The offset in Y direction
        int32_t OffsetY = 0;
    };

    class EventInputMouseMoved : public EventInput
    {
      public:
        inline EventInputMouseMoved() = default;
        inline EventInputMouseMoved(Window* const source, const uint32_t timestamp, InputDevice* const device, fp32_t currentx, fp32_t currenty, fp32_t relativeX, fp32_t relativeY)
            : EventInput(source, timestamp, EType::InputMouseMoved, device), CurrentX(currentx), CurrentY(currenty), RelativeX(relativeX), RelativeY(relativeY)
        {
        }

        /// @brief Current mouse x position
        fp32_t CurrentX = 0.f;
        /// @brief Current mouse y position
        fp32_t CurrentY = 0.f;
        /// @brief Mouse x relative movement since last event
        fp32_t RelativeX = 0.f;
        /// @brief Mouse y relative movement since last event
        fp32_t RelativeY = 0.f;
    };

    class EventWindowResized : public Event
    {
      public:
        inline EventWindowResized() = default;
        inline EventWindowResized(Window* const source, const uint32_t timestamp, VkExtent2D current) : Event(source, timestamp, EType::WindowResized), Current(current) {}

        /// @brief Current window extent
        VkExtent2D Current = {};
    };

    class EventWindowFocusChanged : public Event
    {
      public:
        inline EventWindowFocusChanged() = default;
        inline EventWindowFocusChanged(Window* source, const uint32_t timestamp, bool mouseFocus, bool inputFocus)
            : Event(source, timestamp, EType::WindowFocusChanged), MouseFocus(mouseFocus), InputFocus(inputFocus)
        {
        }

        /// @brief True, if window has mouse focus (The window itself may not be focused, but mouse hovering above)
        bool MouseFocus = false;
        /// @brief True, if window has full focus
        bool InputFocus = false;
    };

    class EventWindowCloseRequested : public Event
    {
      public:
        inline EventWindowCloseRequested() = default;
        inline EventWindowCloseRequested(Window* const source, const uint32_t timestamp) : Event(source, timestamp, EType::WindowCloseRequested) {}
    };

    class EventWindowItemDropped : public Event
    {
      public:
        inline EventWindowItemDropped() = default;
        inline EventWindowItemDropped(Window* const source, const uint32_t timestamp, std::string_view path) : Event(source, timestamp, EType::WindowItemDropped), Path(path) {}

        /// @brief Path to the file dropped
        std::string Path = nullptr;
    };
}  // namespace foray