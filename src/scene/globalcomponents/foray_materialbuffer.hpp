#pragma once
#include "../../util/foray_managedvectorbuffer.hpp"
#include "../foray_component.hpp"
#include "../foray_material.hpp"

namespace foray::scene {

    /// @brief Manages a device local storage buffer providing material information for rendering
    class MaterialBuffer : public GlobalComponent
    {
      public:
        explicit MaterialBuffer(core::Context* context);

        std::vector<DefaultMaterialEntry>& GetVector() { return mBuffer.GetVector(); }

        /// @brief Apply changes made to the cpu local buffer to the device local buffer
        void UpdateDeviceLocal();
        void Destroy();

        inline virtual ~MaterialBuffer() { Destroy(); }

        FORAY_PROPERTY_ALLGET(Buffer)

        inline VkDescriptorBufferInfo GetVkDescriptorInfo() const { return mBuffer.GetBuffer().GetVkDescriptorBufferInfo(); }
        inline VkBuffer               GetVkBuffer() const { return mBuffer.GetBuffer().GetBuffer(); }


      protected:
        util::ManagedVectorBuffer<DefaultMaterialEntry> mBuffer = {};
    };
}  // namespace foray::scene