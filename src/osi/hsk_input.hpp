#pragma once
#include "hsk_helpers.hpp"
#include "hsk_osi_declares.hpp"

namespace hsk {
    class InputBase
    {
      public:
        /// @brief Device this input is part of
        inline InputDevice*       Device() { return mDevice; }
        inline const InputDevice* Device() const { return mDevice; }
        /// @brief Device-unique Id of the input
        inline int32_t Id() const { return mId; }
        /// @brief Human readable name of the input
        inline std::string_view Name() const { return mName; }

      protected:
        InputDevice* mDevice = {};
        int32_t      mId     = {};
        std::string  mName   = {};

        inline InputBase() = default;
        inline InputBase(InputDevice* device, int32_t id, std::string_view name) : mDevice(device), mId(id), mName(name) {}
    };

    /// @brief Represents a single input with a state represented by a signed 16bit integer
    class InputAnalogue : public InputBase
    {
      public:
        /// @brief Axis id of the input
        inline EAxis Axis() const { return mAxis; }

        /// @brief Fetches the inputs current state
        virtual int16_t State() const = 0;

      protected:
        EAxis mAxis = {};

        InputAnalogue() : InputBase(), mAxis() {}
        InputAnalogue(InputDevice* device, int32_t id, std::string_view name, EAxis axis) : InputBase(device, id, name), mAxis(axis) {}
    };

    /// @brief Represents a single input with a state represented by a boolean value
    class InputBinary : public InputBase
    {
      public:
        /// @brief Button id of the input
        inline EButton Button() const { return mButton; }

        /// @brief Fetches the inputs current state
        virtual bool State() const = 0;

      protected:
        EButton mButton = {};

        InputBinary() : InputBase(), mButton() {}
        InputBinary(InputDevice* device, int32_t id, std::string_view name, EButton button) : InputBase(device, id, name), mButton(button) {}
    };

    /// @brief Represents a stateless directional input
    class InputDirectional : public InputBase
    {
      public:
        HSK_PROPERTY_CGET(Directional)

      protected:
        EDirectional mDirectional;
        InputDirectional() : InputBase(), mDirectional() {}
        InputDirectional(InputDevice* device, int32_t id, std::string_view name, EDirectional directional) : InputBase(device, id, name), mDirectional(directional) {}

    };

}  // namespace hsk