#pragma once
#include "hsk_osi_declares.hpp"

namespace hsk
{
    class InputAnalogue
    {
    public:
        loan_ptr<InputDevice> Device;
        int32_t Id;
        std::string Name;
        EAxis Axis;

    protected:
        InputAnalogue() : Device(), Id(-1), Name(), Axis() {}
        InputAnalogue(loan_ptr<InputDevice> device, int32_t id, std::string_view name, EAxis axis) : Device(device), Id(id), Name(name), Axis(axis) {}

    public:
        virtual int16_t State() const = 0;
    };

    class InputBinary
    {
    public:
        loan_ptr<InputDevice> Device;
        int32_t Id;
        std::string Name;
        EButton Button;

    protected:
        InputBinary() : Device(), Id(), Name(), Button() {}
        InputBinary(loan_ptr<InputDevice> device, int32_t id, std::string_view name, EButton button) : Device(device), Id(id), Name(name), Button(button) {}

    public:
        virtual bool State() const = 0;
    };

}