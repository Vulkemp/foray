#pragma once
#include "../hsk_basics.hpp"
#include "hsk_helpers.hpp"
#include "hsk_input.hpp"
#include "hsk_osi_declares.hpp"
#include <memory>

namespace hsk {
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
            /// @brief An input device was connected or removed
            InputDeviceAvailability,
            /// @brief Custom type
            Custom,
            ENUM_MAX
        };

        /// @brief Source window that recorded the event, if applicable
        inline Window* const       Source() { return mSource; }
        inline const Window* const Source() const { return mSource; }
        /// @brief Timestamp when the action was recorded
        inline uint32_t Timestamp() const { return mTimestamp; }
        /// @brief EType enum for aiding in type casting
        inline EType Type() const { return mType; }
        /// @brief For custom event type overloads, this value may be set
        inline int8_t CustomType() const { return mCustomType; }


        inline Event(Window* const source, const uint32_t timestamp, const EType type) : mSource(source), mTimestamp(timestamp), mType(type), mCustomType(0) {}
        inline Event(Window* const source, const uint32_t timestamp, const int8_t customtype)
            : mSource(source), mTimestamp(timestamp), mType(EType::Custom), mCustomType(customtype)
        {
        }

        inline Event(const Event& other) = default;
        inline Event& operator=(const Event& other) = default;

        virtual ~Event() {}

      protected:
        Window* const  mSource;
        const uint32_t mTimestamp;
        const EType    mType;
        int8_t         mCustomType;
    };

    class EventInput : public Event
    {
      public:
        using ptr = std::shared_ptr<EventInput>;

      public:
        /// @brief Input Device that the input was read from
        inline InputDevice* const       Device() { return mDevice; }
        inline const InputDevice* const Device() const { return mDevice; }

        inline EventInput(Window* const source, const uint32_t timestamp, const EType type, InputDevice* const device) : Event(source, timestamp, type), mDevice(device) {}

      protected:
        InputDevice* const mDevice;
    };

    class EventInputDeviceAvailability : public EventInput
    {
      public:
        using ptr = std::shared_ptr<EventInput>;

      public:
        inline bool Added() const { return mAdded; }

        inline EventInputDeviceAvailability(const uint32_t timestamp, InputDevice* const device, const bool added)
            : EventInput(nullptr, timestamp, Event::EType::InputDeviceAvailability, device), mAdded(added)
        {
        }

      protected:
        const bool mAdded;
    };

    class EventInputAnalogue : public EventInput
    {
      public:
        using ptr = std::shared_ptr<EventInputAnalogue>;

      public:
        /// @brief axis that was moved
        inline const InputAnalogue* const Axis() const { return mAxis; }
        /// @brief Current reading from the axis
        inline int16_t State() const { return mState; }

        inline EventInputAnalogue(Window* const source, const uint32_t timestamp, InputDevice* device, const InputAnalogue* axis, fp32_t current)
            : EventInput(source, timestamp, EType::InputAnalogue, device), mAxis(axis), mState(current)
        {
        }

      protected:
        const InputAnalogue* const mAxis;
        const int16_t              mState;
    };

    class EventInputBinary : public EventInput
    {
      public:
        using ptr = std::shared_ptr<EventInputBinary>;

      public:
        /// @brief The button that was pressed or released
        inline const InputBinary* const Button() const { return mButton; }
        /// @brief If true, the button was pressed - released otherwise
        inline bool Pressed() const { return mPressed; }

        inline EventInputBinary(Window* const source, const uint32_t timestamp, InputDevice* device, const InputBinary* button, bool pressed)
            : EventInput(source, timestamp, EType::InputBinary, device), mButton(button), mPressed(pressed)
        {
        }

      protected:
        const InputBinary* const mButton;
        const bool               mPressed;
    };

    class EventInputMouseMoved : public EventInput
    {
      public:
        using ptr = std::shared_ptr<EventInputMouseMoved>;

      public:
        inline fp32_t CurrentX() const { return mCurrentX; }
        inline fp32_t CurrentY() const { return mCurrentY; }

        EventInputMouseMoved(Window* const source, const uint32_t timestamp, InputDevice* const device, fp32_t currentx, fp32_t currenty)
            : EventInput(source, timestamp, EType::InputMouseMoved, device), mCurrentX(currentx), mCurrentY(currenty)
        {
        }

      protected:
        fp32_t mCurrentX;
        fp32_t mCurrentY;
    };

    class EventWindowResized : public Event
    {
      public:
        using ptr = std::shared_ptr<EventWindowResized>;

      public:
        inline Extent2D Current() const { return mCurrent; }

        EventWindowResized(Window* const source, const uint32_t timestamp, Extent2D current) : Event(source, timestamp, EType::WindowResized), mCurrent(current) {}

      protected:
        const Extent2D mCurrent;
    };

    class EventWindowFocusChanged : public Event
    {
      public:
        using ptr = std::shared_ptr<EventWindowFocusChanged>;

      public:
        inline bool MouseFocus() const { return mMouseFocus; }
        inline bool InputFocus() const { return mInputFocus; }

        EventWindowFocusChanged(Window* source, const uint32_t timestamp, bool mouseFocus, bool inputFocus)
            : Event(source, timestamp, EType::WindowFocusChanged), mMouseFocus(mouseFocus), mInputFocus(inputFocus)
        {
        }

      protected:
        const bool mMouseFocus;
        const bool mInputFocus;
    };

    class EventWindowCloseRequested : public Event
    {
      public:
        using ptr = std::shared_ptr<EventWindowCloseRequested>;

        EventWindowCloseRequested(Window* const source, const uint32_t timestamp) : Event(source, timestamp, EType::WindowCloseRequested) {}
    };

    class EventWindowItemDropped : public Event
    {
      public:
        using ptr = std::shared_ptr<EventWindowItemDropped>;

      public:
        const char* const Path() const { return mPath; }

        EventWindowItemDropped(Window* const source, const uint32_t timestamp, const char* path) : Event(source, timestamp, EType::WindowItemDropped), mPath(path) {}

      protected:
        const char* const mPath;
    };
}  // namespace hsk