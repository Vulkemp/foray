#pragma once
#include "hsk_osi_declares.hpp"

namespace hsk {
    /// @brief Represents a single input with a state represented by a signed 16bit integer
    class InputAnalogue
    {
      public:
        /// @brief Device this input is part of
        loan_ptr<InputDevice> Device;
        /// @brief Device-unique Id of the input
        int32_t Id;
        /// @brief Human readable name of the input
        std::string Name;
        /// @brief Axis id of the input
        EAxis Axis;

      protected:
        InputAnalogue() : Device(), Id(-1), Name(), Axis() {}
        InputAnalogue(loan_ptr<InputDevice> device, int32_t id, std::string_view name, EAxis axis) : Device(device), Id(id), Name(name), Axis(axis) {}

      public:
        /// @brief Fetches the inputs current state
        virtual int16_t State() const = 0;
    };

    /// @brief Represents a single input with a state represented by a boolean value
    class InputBinary
    {
      public:
        /// @brief Device this input is part of
        loan_ptr<InputDevice> Device;
        /// @brief Device-unique Id of the input
        int32_t Id;
        /// @brief Human readable name of the input
        std::string Name;
        /// @brief Button id of the input
        EButton Button;

      protected:
        InputBinary() : Device(), Id(), Name(), Button() {}
        InputBinary(loan_ptr<InputDevice> device, int32_t id, std::string_view name, EButton button) : Device(device), Id(id), Name(name), Button(button) {}

      public:
        /// @brief Fetches the inputs current state
        virtual bool State() const = 0;
    };

}  // namespace hsk