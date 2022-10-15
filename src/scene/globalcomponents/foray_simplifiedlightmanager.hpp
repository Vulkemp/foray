#pragma once
#include "../../core/foray_descriptorsethelper.hpp"
#include "../../util/foray_dualbuffer.hpp"
#include "../components/foray_simplifiedlightcomponent.hpp"
#include "../foray_component.hpp"
#include "../foray_simplifiedlight.hpp"
#include <unordered_map>

namespace foray::scene {
    class SimplifiedLightManager : public GlobalComponent, public Component::UpdateCallback
    {
      public:
        void CreateOrUpdate();

        virtual void Update(const base::FrameRenderInfo& updateInfo) override;

        virtual int32_t GetOrder() const override { return ORDER_DEVICEUPLOAD; }

      protected:
        std::vector<SimplifiedLight>                            mSimplifiedlights;
        std::unordered_map<SimplifiedLightComponent*, uint32_t> mComponentArrayBindings;
        util::DualBuffer                                              mBuffer;
    };
}  // namespace foray
