#pragma once
#include "../hsk_basics.hpp"
#include "../hsk_memory.hpp"
#include <memory>
#include <glm/glm.hpp>
#include "hsk_fwddeclare.hpp"

namespace hsk{
        /// @brief Event Base class
    class Event
    {
      public:
        using ptr = std::shared_ptr<Event>;

        /// @brief
        enum class EType : int8_t
        {
            Undefined = -1,
            /// @brief Analogue inputs, ex. controller stick axis
            InputAnalogue,
            /// @brief Binary inputs, ex. buttons, keys
            InputBinary,
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
            /// @brief An input device was connected to the computer
            InputDeviceAdded,
            /// @brief An input device was disconnected
            InputDeviceRemoved,
            /// @brief Custom type
            Custom,
            ENUM_MAX
        };

      public:
        /// @brief Source window that recorded the event, if applicable
        const loan_ptr<const Window> Source;
        /// @brief Timestamp when the action was recorded
        const uint32_t Timestamp;
        /// @brief EType enum for aiding in type casting
        const EType Type;
        /// @brief For custom event type overloads, this value may be set
        int8_t CustomType;

        inline Event(loan_ptr<const Window> source, const uint32_t timestamp, const EType type) : Source(source), Timestamp(timestamp), Type(type), CustomType(0) {}
        inline Event(loan_ptr<const Window> source, const uint32_t timestamp, const int8_t customtype)
            : Source(source), Timestamp(timestamp), Type(EType::Custom), CustomType(customtype)
        {
        }

        inline Event(const Event& other)  = default;
        inline Event& operator=(const Event& other) = default;

        virtual ~Event() {}
    };

    class EventInput : public Event
    {
      public:
        using ptr = std::shared_ptr<EventInput>;

        /// @brief Input Device that the input was read from
        const loan_ptr<const InputDevice> Device;

        inline EventInput(loan_ptr<const Window> source, const uint32_t timestamp, const EType type, loan_ptr<const InputDevice> device) : Event(source, timestamp, type), Device(device) {}
    };

    class EventInputAnalogue : public EventInput
    {
      public:
        using ptr = std::shared_ptr<EventInputAnalogue>;

        /// @brief id of the axis that was moved
        const uint32_t AxisId;
        /// @brief Current reading from the axis
        const fp64_t Current;
        /// @brief Previous reading from the axis
        const fp64_t Before;

        inline EventInputAnalogue(loan_ptr<const Window> source, const uint32_t timestamp, loan_ptr<const InputDevice> device, uint32_t axis, fp64_t current, fp64_t before)
            : EventInput(source, timestamp, EType::InputAnalogue, device), AxisId(axis), Current(current), Before(before)
        {
        }

        fp64_t Delta() const { return Before - Current; }
    };


    class EventInputBinary : public EventInput
    {
      public:
        using ptr = std::shared_ptr<EventInputBinary>;

        /// @brief The button that was pressed or released
        const EButton Button;
        /// @brief If true, the button was pressed - released otherwise
        const bool Pressed;

        inline EventInputBinary(loan_ptr<const Window> source, const uint32_t timestamp, loan_ptr<const InputDevice> device, EButton button, bool pressed)
            : EventInput(source, timestamp, EType::InputBinary, device), Button(button), Pressed(pressed)
        {
        }
    };

    class EventInputMouseMoved : public EventInput
    {
      public:
        using ptr = std::shared_ptr<EventInputMouseMoved>;

        glm::vec2 m_Current;
        glm::vec2 m_Before;

        EventInputMouseMoved(loan_ptr<const Window> source, const uint32_t timestamp, loan_ptr<const InputDevice> device, glm::vec2 current, glm::vec2 before)
            : EventInput(source, timestamp, EType::InputMouseMoved, device), m_Current(current), m_Before(before)
        {
        }
    };

    class EventWindowResized : public Event
    {
      public:
        using ptr = std::shared_ptr<EventWindowResized>;

        const Extent2D Current;

        EventWindowResized(loan_ptr<const Window> source, const uint32_t timestamp, Extent2D current) : Event(source, timestamp, EType::WindowResized), Current(current) {}
    };

    class EventWindowFocusChanged : public Event
    {
      public:
        using ptr = std::shared_ptr<EventWindowFocusChanged>;

        const bool MouseFocus;
        const bool InputFocus;

        EventWindowFocusChanged(loan_ptr<const Window> source, const uint32_t timestamp, bool mouseFocus, bool inputFocus) : Event(source, timestamp, EType::WindowFocusChanged), MouseFocus(mouseFocus), InputFocus(inputFocus) {}
    };

    class EventWindowCloseRequested : public Event
    {
      public:
        using ptr = std::shared_ptr<EventWindowCloseRequested>;

        EventWindowCloseRequested(loan_ptr<const Window> source, const uint32_t timestamp) : Event(source, timestamp, EType::WindowCloseRequested) {}
    };

    class EventWindowItemDropped : public Event
    {
      public:
        using ptr = std::shared_ptr<EventWindowItemDropped>;

        const char*const m_Path;

        EventWindowItemDropped(loan_ptr<const Window> source, const uint32_t timestamp, const char* path) : Event(source, timestamp, EType::WindowItemDropped), m_Path(path) {}
    };
}