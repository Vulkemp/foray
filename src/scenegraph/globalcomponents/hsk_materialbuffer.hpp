#pragma once
#include "../../memory/hsk_managedvectorbuffer.hpp"
#include "../hsk_component.hpp"
#include "../hsk_material.hpp"

namespace hsk {

    /// @brief Manages a device local storage buffer providing material information for rendering
    class NMaterialBuffer : public GlobalComponent
    {
      public:
        explicit NMaterialBuffer(const VkContext* context);

        std::vector<NMaterialBufferObject>& GetVector() { return mBuffer.GetVector(); }

        /// @brief Apply changes made to the cpu local buffer to the device local buffer
        void UpdateDeviceLocal();
        void Cleanup();

        std::shared_ptr<DescriptorSetHelper::DescriptorInfo> MakeDescriptorInfo();

        inline virtual ~NMaterialBuffer() { Cleanup(); }

      protected:
        ManagedVectorBuffer<NMaterialBufferObject> mBuffer = {};
    };
}  // namespace hsk