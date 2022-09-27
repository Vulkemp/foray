#include "hsk_rtpipeline.hpp"

namespace hsk {






    // uint32_t RtPipeline::AddShaderGroupRaygen(ShaderModule* module)
    // {
    //     ShaderGroupId groupId = (ShaderGroupId)mShaderGroups.size();
    //     SbtBindId     index   = mRaygenSbt.SetGroup()
    //     mShaderGroups.push_back(ShaderGroup{.Id = groupId, .Type = RtShaderGroupType::Raygen, .General = module});
    //     return groupId;
    // }
    // uint32_t RtPipeline::AddShaderGroupCallable(ShaderModule* module)
    // {
    //     ShaderGroupId groupId = (ShaderGroupId)mShaderGroups.size();
    //     SbtBindId     index   = mCallablesSbt.SetGroup(module, RtShaderType::Callable, groupId);
    //     mShaderGroups.push_back(ShaderGroup{.Id = groupId, .Type = RtShaderGroupType::Callable, .General = module});
    //     return groupId;
    // }
    // uint32_t RtPipeline::AddShaderGroupMiss(ShaderModule* module)
    // {
    //     ShaderGroupId groupId = (ShaderGroupId)mShaderGroups.size();
    //     SbtBindId     index   = mMissSbt.SetGroup(module, RtShaderType::Miss, groupId);
    //     mShaderGroups.push_back(ShaderGroup{.Id = groupId, .Type = RtShaderGroupType::Miss, .General = module});
    //     return groupId;
    // }
    // uint32_t RtPipeline::AddShaderGroupIntersect(ShaderModule* closestHit, ShaderModule* anyHit, ShaderModule* intersect)
    // {
    //     ShaderGroupId groupId = (ShaderGroupId)mShaderGroups.size();
    //     ShaderGroup   group   = ShaderGroup{.Id = groupId, .Type = RtShaderGroupType::Intersect};
    //     if(!!closestHit)
    //     {
    //         mIntersectsSbt.SetGroup(closestHit, RtShaderType::ClosestHit, groupId);
    //         group.ClosestHit = closestHit;
    //     }
    //     if(!!anyHit)
    //     {
    //         mIntersectsSbt.SetGroup(anyHit, RtShaderType::Anyhit, groupId);
    //         group.AnyHit = anyHit;
    //     }
    //     if(!!intersect)
    //     {
    //         mIntersectsSbt.SetGroup(intersect, RtShaderType::Intersect, groupId);
    //         group.Intersect = intersect;
    //     }
    //     mShaderGroups[groupId] = group;
    //     return groupId;
    // }

    struct ShaderGroupHandles
    {
        ShaderHandle General    = {};
        ShaderHandle ClosestHit = {};
        ShaderHandle AnyHit     = {};
        ShaderHandle Intersect  = {};
    };

    void RtPipeline::Build(const VkContext* context)
    {
        /// STEP # 0    Reset, get physical device properties

        mShaderCollection.Clear();

        VkPhysicalDeviceRayTracingPipelinePropertiesKHR pipelineProperties{.sType = VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR};
        {
            VkPhysicalDeviceProperties2 prop2{};
            prop2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
            prop2.pNext = &pipelineProperties;
            vkGetPhysicalDeviceProperties2(context->PhysicalDevice, &prop2);
        }


        /// STEP # 1    Rebuild shader collection

        mRaygenSbt.WriteToShaderCollection(mShaderCollection);
        mMissSbt.WriteToShaderCollection(mShaderCollection);
        mCallablesSbt.WriteToShaderCollection(mShaderCollection);
        mIntersectsSbt.WriteToShaderCollection(mShaderCollection);

        mShaderCollection.BuildShaderStageCiVector();


        /// STEP # 2    Collect ShaderGroup, build sorted vector

        std::vector<VkRayTracingShaderGroupCreateInfoKHR> shaderGroupCis;

        // Insert grouped into shaderGroupCis vector

        

        uint32_t missOffset      = 0;
        uint32_t callablesOffset = 0;
        uint32_t intersectOffset = 0;

        mRaygenSbt.WriteToShaderGroupCiVector(shaderGroupCis, mShaderCollection);
        missOffset = shaderGroupCis.size();
        mMissSbt.WriteToShaderGroupCiVector(shaderGroupCis, mShaderCollection);
        callablesOffset = shaderGroupCis.size();
        mCallablesSbt.WriteToShaderGroupCiVector(shaderGroupCis, mShaderCollection);
        intersectOffset = shaderGroupCis.size();
        mIntersectsSbt.WriteToShaderGroupCiVector(shaderGroupCis, mShaderCollection);


        /// STEP # 3    Build RT pipeline

        // Create the ray tracing pipeline
        VkRayTracingPipelineCreateInfoKHR raytracingPipelineCreateInfo{
            .sType                        = VkStructureType::VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR,
            .stageCount                   = static_cast<uint32_t>(mShaderCollection.GetShaderStageCis().size()),
            .pStages                      = mShaderCollection.GetShaderStageCis().data(),
            .groupCount                   = static_cast<uint32_t>(shaderGroupCis.size()),
            .pGroups                      = shaderGroupCis.data(),
            .maxPipelineRayRecursionDepth = pipelineProperties.maxRayRecursionDepth,
            .layout                       = mPipelineLayout,
        };

        AssertVkResult(context->DispatchTable.createRayTracingPipelinesKHR(nullptr, nullptr, 1, &raytracingPipelineCreateInfo, nullptr, &mPipeline));


        /// STEP # 4    Get shader handles

        std::vector<uint8_t> shaderHandleData(shaderGroupCis.size() * pipelineProperties.shaderGroupHandleSize);
        AssertVkResult(context->DispatchTable.getRayTracingShaderGroupHandlesKHR(mPipeline, 0, shaderGroupCis.size(), shaderHandleData.size(), shaderHandleData.data()));

        std::vector<ShaderHandle> shaderHandles;
        shaderHandles.reserve(mShaderGroups.size());

        {
            for(const GeneralShaderBindingTable::ShaderGroup& shaderRef : mRaygenSbt.GetShaders())
            {
                const uint8_t* dataPtr = shaderHandleData.data() + (shaderRef.GroupId * pipelineProperties.shaderGroupHandleSize);
                ShaderHandle   handle  = {};
                memcpy(&handle, dataPtr, pipelineProperties.shaderGroupHandleSize);
                shaderHandles.push_back(handle);
            }
            mRaygenSbt.Build(context, pipelineProperties, shaderHandles);
        }
        {
            for(const GeneralShaderBindingTable::ShaderGroup& shaderRef : mCallablesSbt.GetShaders())
            {
                const uint8_t* dataPtr = shaderHandleData.data() + (shaderRef.GroupId * pipelineProperties.shaderGroupHandleSize);
                ShaderHandle   handle  = {};
                memcpy(&handle, dataPtr, pipelineProperties.shaderGroupHandleSize);
                shaderHandles.push_back(handle);
            }
            mRaygenSbt.Build(context, pipelineProperties, shaderHandles);
        }
    }
}  // namespace hsk
