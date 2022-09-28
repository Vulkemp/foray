#pragma once
#include "../memory/hsk_managedbuffer.hpp"
#include "../utility/hsk_shadermodule.hpp"
#include "hsk_rtpipeline_declares.hpp"
#include "hsk_rtshadertypes.hpp"
#include <unordered_map>

namespace hsk {
    class ShaderBindingTableBase
    {
      public:
        explicit ShaderBindingTableBase(VkDeviceSize entryDataSize = 0);

        struct VectorRange
        {
            int32_t Start = -1;
            int32_t Count = 0;
        };

        virtual void Build(const VkContext*                                       context,
                           const VkPhysicalDeviceRayTracingPipelinePropertiesKHR& pipelineProperties,
                           const std::unordered_map<GroupIndex, const uint8_t*>&  handles);

        HSK_PROPERTY_CGET(EntryDataSize)
        HSK_PROPERTY_ALLGET(AddressRegion)
        HSK_PROPERTY_ALLGET(Buffer)

        std::vector<uint8_t>&       GroupDataAt(GroupIndex groupIndex);
        const std::vector<uint8_t>& GroupDataAt(GroupIndex groupIndex) const;

        template <typename TData>
        TData& GroupDataAt(GroupIndex groupIndex)
        {
            std::vector<uint8_t>& data = GroupDataAt(groupIndex);
            Assert(sizeof(TData) == data.size(), "");
            return *(reinterpret_cast<TData*>(GroupDataAt(index)));
        }
        template <typename TData>
        TData& GroupDataAt(GroupIndex groupIndex) const
        {
            return *(reinterpret_cast<TData*>(GroupDataAt(index)));
        }

        virtual ShaderBindingTableBase& SetEntryDataSize(VkDeviceSize size);

        virtual void        WriteToShaderCollection(RtShaderCollection& collection) const                                                                             = 0;
        virtual VectorRange WriteToShaderGroupCiVector(std::vector<VkRayTracingShaderGroupCreateInfoKHR>& groupCis, const RtShaderCollection& shaderCollection) const = 0;

        virtual ~ShaderBindingTableBase();

        virtual size_t GetGroupArrayCount() const = 0;

      protected:
        ManagedBuffer                   mBuffer;
        VkStridedDeviceAddressRegionKHR mAddressRegion{};

        VkDeviceSize mEntryDataSize{};

        std::vector<std::vector<uint8_t>> mGroupData{};

        void ArrayResized(size_t newSize);
        void SetData(GroupIndex index, const void* data);
    };
}  // namespace hsk
