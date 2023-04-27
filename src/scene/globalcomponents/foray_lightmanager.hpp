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
        inline LightManager() : Component::UpdateCallback(ORDER_DEVICEUPLOAD) {}

        /// @brief Collects lights and reuploads to device
        void CreateOrUpdate();

        virtual void Update(SceneUpdateInfo& updateInfo) override;

        FORAY_PROPERTY_R(Buffer)

      protected:
        std::vector<SimpleLight>                            mSimplifiedlights;
        std::unordered_map<ncomp::PunctualLight*, uint32_t> mComponentArrayBindings;
        Local<util::DualBuffer>                                    mBuffer;
    };
}  // namespace foray::scene::gcomp
