#include "hsk_inputdevice.hpp"
#include <sstream>

namespace hsk
{
    const InputDevice::ButtonPtr InputDevice::FindButton(EButton button) const
    {
        for (ButtonPtr buttonptr : mButtons)
        {
            if (buttonptr->Type == button)
            {
                return buttonptr;
            }
        }
        return nullptr;
    }
    std::string InputDevice::BuildDebugPrint() const
    {
        std::stringstream builder;

        builder << Name() << "\n";
        if (mButtons.size() > 0)
        {
            builder << "\t" << mButtons.size() << " Buttons:\n";
            for (ButtonPtr button : mButtons)
            {
                builder << "\t\t" << button->Name << "\n";
            }
        }
        if (mAxes.size() > 0)
        {
            builder << "\t" << mAxes.size() << " Axes:\n";
            for (AxisPtr axis : mAxes)
            {
                builder << "\t\t" << axis->Name << "\n";
            }
        }

        return builder.str();
    }

}