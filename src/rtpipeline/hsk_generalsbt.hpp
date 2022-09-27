#pragma once
#include "hsk_basesbt.hpp"
#include "hsk_rtshadertypes.hpp"

namespace hsk
{
        /// @brief Shader binding table for storing raygen, miss and callable shader groups
    class GeneralShaderBindingTable : public ShaderBindingTableBase
    {
      public:
        /*
          A general shader binding table stores entries in the following layout

          | Entry0                      | Entry1                      | ...
          |-----------------------------|-----------------------------|-------
          | ShaderHandle | Custom Data  | ShaderHandle | Custom Data  | ...

          Where the shader handle is a vulkan driver specific memory block (all currently known drivers use 
          32 bit values here), as defined by VkPhysicalDeviceRayTracingPipelinePropertiesKHR::shaderGroupHandleSize

          Custom Data is an optional memory area of user defined size, used to initialize indices. It allows pairing
          shaders with different configuration parameters.

          For ClosestHit, AnyHit, Intersection shader groups refer to the IntersectShaderBindingTable type
        */

        struct ShaderGroup
        {
            ShaderModule* Module  = nullptr;
            int32_t       Index   = -1;
            ShaderGroupId GroupId = -1;
        };

        explicit GeneralShaderBindingTable(RtShaderGroupType groupType, VkDeviceSize entryDataSize = 0);

        /// @brief Add a shader without custom data
        void SetGroup(int32_t index, ShaderGroupId groupId, ShaderModule* shader);
        /// @brief Add shader with custom data
        /// @param data pointer to a memory area of mEntryDataSize size. Use SetShaderDataSize(...) before adding shaders!
        void SetGroup(int32_t index, ShaderGroupId groupId, ShaderModule* shader, const void* data);

        /// @brief Creates / updates the buffer
        virtual void Build(const VkContext*                                         context,
                           const VkPhysicalDeviceRayTracingPipelinePropertiesKHR&   pipelineProperties,
                           const std::unordered_map<ShaderModule*, const uint8_t*>& handles);

        HSK_PROPERTY_ALLGET(Groups)

        virtual void WriteToShaderCollection(RtShaderCollection& collection) const;
        virtual void WriteToShaderGroupCiVector(std::vector<VkRayTracingShaderGroupCreateInfoKHR>& groupCis, const RtShaderCollection& shaderCollection) const;

        virtual ~GeneralShaderBindingTable();

        inline virtual size_t GetGroupArrayCount() const { return mGroups.size(); }


      protected:
        RtShaderGroupType        mShaderGroupType = RtShaderGroupType::Undefined;
        std::vector<ShaderGroup> mGroups;
    };
} // namespace hsk
