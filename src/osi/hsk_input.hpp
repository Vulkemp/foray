#pragma once
#include "hsk_helpers.hpp"
#include "hsk_osi_declares.hpp"

namespace hsk {
    class InputBase
    {
      public:
        /// @brief Device providing this input
        HSK_PROPERTY_ALLGET(Device)
        /// @brief Device-unique Id of the input
        HSK_PROPERTY_ALLGET(Id)
        /// @brief Human readable name of the input
        HSK_PROPERTY_ALLGET(Name)

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
        HSK_PROPERTY_ALLGET(AxisId)

        /// @brief Fetches the inputs current state
        virtual int16_t State() const = 0;

      protected:
        EAxis mAxisId = {};

        InputAnalogue() : InputBase(), mAxisId() {}
        InputAnalogue(InputDevice* device, int32_t id, std::string_view name, EAxis axis) : InputBase(device, id, name), mAxisId(axis) {}
    };

    /// @brief Represents a single input with a state represented by a boolean value
    class InputBinary : public InputBase
    {
      public:
        /// @brief Button id of the input
        HSK_PROPERTY_ALLGET(ButtonId)

        /// @brief Fetches the inputs current state
        virtual bool State() const = 0;

      protected:
        EButton mButtonId = {};

        InputBinary() : InputBase(), mButtonId() {}
        InputBinary(InputDevice* device, int32_t id, std::string_view name, EButton button) : InputBase(device, id, name), mButtonId(button) {}
    };

    /// @brief Represents a stateless directional input
    class InputDirectional : public InputBase
    {
      public:
        /// @brief Directional input id of the inptu
        HSK_PROPERTY_CGET(DirectionalId)

      protected:
        EDirectional mDirectionalId;
        InputDirectional() : InputBase(), mDirectionalId() {}
        InputDirectional(InputDevice* device, int32_t id, std::string_view name, EDirectional directional) : InputBase(device, id, name), mDirectionalId(directional) {}
    };

}  // namespace hsk