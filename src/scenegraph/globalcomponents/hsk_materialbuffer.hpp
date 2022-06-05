#pragma once
#include "../../memory/hsk_managedvectorbuffer.hpp"
#include "../hsk_component.hpp"
#include "../hsk_material.hpp"

namespace hsk {
    class NMaterialBuffer : public Component
    {
      public:
        explicit NMaterialBuffer(const VkContext* context);

        std::vector<NMaterialBufferObject>& GetVector() { return mBuffer.GetVector(); }

        void UpdateDeviceLocal();
        void Cleanup();

        inline virtual ~NMaterialBuffer() { Cleanup(); }

      protected:
        ManagedVectorBuffer<NMaterialBufferObject> mBuffer = {};
    };
}  // namespace hsk