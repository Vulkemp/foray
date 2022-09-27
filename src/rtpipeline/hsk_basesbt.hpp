#pragma once
#include "../memory/hsk_managedbuffer.hpp"
#include "../utility/hsk_shadermodule.hpp"
#include "hsk_rtpipeline_declares.hpp"
#include <unordered_map>

namespace hsk {
    class ShaderBindingTableBase
    {
      public:
        explicit ShaderBindingTableBase(VkDeviceSize entryDataSize = 0);

        virtual void Build(const VkContext*                                         context,
                           const VkPhysicalDeviceRayTracingPipelinePropertiesKHR&   pipelineProperties,
                           const std::unordered_map<ShaderModule*, const uint8_t*>& handles) = 0;

        HSK_PROPERTY_CGET(EntryDataSize)
        HSK_PROPERTY_ALLGET(AddressRegion)
        HSK_PROPERTY_ALLGET(Buffer)

        void*       GroupDataAt(int32_t index);
        const void* GroupDataAt(int32_t index) const;

        template <typename TData>
        TData& GroupDataAt(int32_t index)
        {
            return *(reinterpret_cast<TData*>(GroupDataAt(index)));
        }
        template <typename TData>
        TData& GroupDataAt(int32_t index) const
        {
            return *(reinterpret_cast<TData*>(GroupDataAt(index)));
        }

        virtual ShaderBindingTableBase& SetEntryDataSize(VkDeviceSize size);

        virtual void WriteToShaderCollection(RtShaderCollection& collection) const                                                                             = 0;
        virtual void WriteToShaderGroupCiVector(std::vector<VkRayTracingShaderGroupCreateInfoKHR>& groupCis, const RtShaderCollection& shaderCollection) const = 0;

        virtual ~ShaderBindingTableBase();

        virtual size_t GetGroupArrayCount() const = 0;

      protected:
        ManagedBuffer                   mBuffer;
        VkStridedDeviceAddressRegionKHR mAddressRegion{};

        VkDeviceSize mEntryDataSize{};

        std::vector<uint8_t> mGroupData{};

        void ArrayResized(size_t newSize);
        void SetData(int32_t index, const void* data);
    };
}  // namespace hsk
