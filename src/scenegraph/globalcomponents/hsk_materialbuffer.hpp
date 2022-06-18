#pragma once
#include "../../memory/hsk_managedvectorbuffer.hpp"
#include "../hsk_component.hpp"
#include "../hsk_material.hpp"
#include "../../memory/hsk_descriptorsethelper.hpp"

namespace hsk {

    /// @brief Manages a device local storage buffer providing material information for rendering
    class MaterialBuffer : public GlobalComponent
    {
      public:
        explicit MaterialBuffer(const VkContext* context);

        std::vector<MaterialBufferEntry>& GetVector() { return mBuffer.GetVector(); }

        /// @brief Apply changes made to the cpu local buffer to the device local buffer
        void UpdateDeviceLocal();
        void Cleanup();

        std::shared_ptr<DescriptorSetHelper::DescriptorInfo> MakeDescriptorInfo();

        inline virtual ~MaterialBuffer() { Cleanup(); }

      protected:
        ManagedVectorBuffer<MaterialBufferEntry> mBuffer = {};
    };
}  // namespace hsk