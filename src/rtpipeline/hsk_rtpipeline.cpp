#include "hsk_rtpipeline.hpp"

namespace hsk {

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

        ShaderBindingTableBase::VectorRange raygen, miss, callable, intersect;

        raygen    = mRaygenSbt.WriteToShaderGroupCiVector(shaderGroupCis, mShaderCollection);
        miss      = mMissSbt.WriteToShaderGroupCiVector(shaderGroupCis, mShaderCollection);
        callable  = mCallablesSbt.WriteToShaderGroupCiVector(shaderGroupCis, mShaderCollection);
        intersect = mIntersectsSbt.WriteToShaderGroupCiVector(shaderGroupCis, mShaderCollection);


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


        /// STEP # 4    Get shader handles, build SBTs

        std::vector<uint8_t> shaderHandleData(shaderGroupCis.size() * pipelineProperties.shaderGroupHandleSize);
        AssertVkResult(context->DispatchTable.getRayTracingShaderGroupHandlesKHR(mPipeline, 0, shaderGroupCis.size(), shaderHandleData.size(), shaderHandleData.data()));

        {
            std::unordered_map<int32_t, const uint8_t*> handles;
            for(int32_t i = 0; i < raygen.Count; i++)
            {
                size_t         handleOffset = (i + raygen.Start) * pipelineProperties.shaderGroupHandleSize;
                const uint8_t* handlePtr    = shaderHandleData.data() + handleOffset;
                handles.emplace(i, handlePtr);
            }
            mRaygenSbt.Build(context, pipelineProperties, handles);
        }
        {
            std::unordered_map<int32_t, const uint8_t*> handles;
            for(int32_t i = 0; i < miss.Count; i++)
            {
                size_t         handleOffset = (i + miss.Start) * pipelineProperties.shaderGroupHandleSize;
                const uint8_t* handlePtr    = shaderHandleData.data() + handleOffset;
                handles.emplace(i, handlePtr);
            }
            mMissSbt.Build(context, pipelineProperties, handles);
        }
        {
            std::unordered_map<int32_t, const uint8_t*> handles;
            for(int32_t i = 0; i < callable.Count; i++)
            {
                size_t         handleOffset = (i + callable.Start) * pipelineProperties.shaderGroupHandleSize;
                const uint8_t* handlePtr    = shaderHandleData.data() + handleOffset;
                handles.emplace(i, handlePtr);
            }
            mCallablesSbt.Build(context, pipelineProperties, handles);
        }
        {
            std::unordered_map<int32_t, const uint8_t*> handles;
            for(int32_t i = 0; i < intersect.Count; i++)
            {
                size_t         handleOffset = (i + intersect.Start) * pipelineProperties.shaderGroupHandleSize;
                const uint8_t* handlePtr    = shaderHandleData.data() + handleOffset;
                handles.emplace(i, handlePtr);
            }
            mIntersectsSbt.Build(context, pipelineProperties, handles);
        }
    }
}  // namespace hsk
