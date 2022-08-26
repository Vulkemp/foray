#pragma once
#include "../../memory/hsk_descriptorsethelper.hpp"
#include "../../memory/hsk_dualbuffer.hpp"
#include "../components/hsk_simplifiedlightcomponent.hpp"
#include "../hsk_component.hpp"
#include "../hsk_simplifiedlight.hpp"
#include <unordered_map>

namespace hsk {
    class SimplifiedLightManager : public GlobalComponent, public Component::UpdateCallback
    {
      public:
        void CreateOrUpdate();

        void Update(const FrameUpdateInfo& updateInfo);

      protected:
        std::vector<SimplifiedLight>                            mSimplifiedlights;
        std::unordered_map<SimplifiedLightComponent*, uint32_t> mComponentArrayBindings;
        DualBuffer                                              mBuffer;
    };
}  // namespace hsk
