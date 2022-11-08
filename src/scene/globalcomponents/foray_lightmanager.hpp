#pragma once
#include "../../util/foray_dualbuffer.hpp"
#include "../components/foray_punctuallight.hpp"
#include "../foray_component.hpp"
#include "../foray_lights.hpp"
#include <unordered_map>

namespace foray::scene::gcomp {
    /// @brief Maintains a list of PunctualLight components
    class LightManager : public GlobalComponent, public Component::UpdateCallback
    {
      public:
        /// @brief Collects lights and reuploads to device
        void CreateOrUpdate();

        virtual void Update(SceneUpdateInfo& updateInfo) override;

        virtual int32_t GetOrder() const override { return ORDER_DEVICEUPLOAD; }

      protected:
        std::vector<SimpleLight>                            mSimplifiedlights;
        std::unordered_map<ncomp::PunctualLight*, uint32_t> mComponentArrayBindings;
        util::DualBuffer                                    mBuffer;
    };
}  // namespace foray::scene::gcomp
