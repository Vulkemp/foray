#pragma once
#include "hsk_basesbt.hpp"
#include "hsk_rtshadertypes.hpp"

namespace hsk {
    class IntersectShaderBindingTable : public ShaderBindingTableBase
    {
      public:
        struct ShaderGroup
        {
            ShaderModule* ClosestHitModule = nullptr;
            ShaderModule* AnyHitModule     = nullptr;
            ShaderModule* IntersectModule  = nullptr;
            GroupIndex    Index            = -1;
        };

        inline explicit IntersectShaderBindingTable(VkDeviceSize entryDataSize = 0) : ShaderBindingTableBase(entryDataSize) {}

        /// @brief Set a shader group without custom data
        void SetGroup(GroupIndex groupIndex, ShaderModule* closestHit, ShaderModule* anyHit, ShaderModule* intersect);
        /// @brief Set a shader group with custom data
        /// @param data pointer to a memory area of mEntryDataSize size. Use SetShaderDataSize(...) before adding shaders!
        void SetGroup(GroupIndex groupIndex, ShaderModule* closestHit, ShaderModule* anyHit, ShaderModule* intersect, const void* data);

        /// @brief Creates / updates the buffer
        virtual void Build(const VkContext*                                       context,
                           const VkPhysicalDeviceRayTracingPipelinePropertiesKHR& pipelineProperties,
                           const std::unordered_map<int32_t, const uint8_t*>&     handles);

        virtual void        WriteToShaderCollection(RtShaderCollection& collection) const;
        virtual VectorRange WriteToShaderGroupCiVector(std::vector<VkRayTracingShaderGroupCreateInfoKHR>& groupCis, const RtShaderCollection& shaderCollection) const;

        inline virtual size_t GetGroupArrayCount() const { return mGroups.size(); }

        HSK_PROPERTY_ALLGET(Groups)

      protected:
        std::vector<ShaderGroup> mGroups;
    };

}  // namespace hsk
