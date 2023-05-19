#include "osi_event.hpp"
#include "osmanager.hpp"

namespace foray::osi
{
    Event::Event(Window* const source, const uint32_t timestamp, const EType type) : Source(source), Timestamp(timestamp), Type(type), CustomType(0) {}
    Event::Event(Window* const source, const uint32_t timestamp, const int8_t customtype) : Source(source), Timestamp(timestamp), Type(EType::Custom), CustomType(customtype) {}

    EventRawSDL::EventRawSDL(const SDL_Event& data) : Event(nullptr, data.common.timestamp, Event::EType::RawSDL), Data(data) {}
} // namespace foray::osi
