#include "foray_rtpipeline.hpp"

namespace foray::rtpipe {

    RtPipeline::RtPipeline() : mRaygenSbt(RtShaderGroupType::Raygen), mMissSbt(RtShaderGroupType::Miss), mCallablesSbt(RtShaderGroupType::Callable), mHitSbt() {}

    void RtPipeline::Build(core::Context* context, VkPipelineLayout pipelineLayout)
    {
        /// STEP # 0    Reset, get physical device properties

        if(!!mPipeline)
        {
            mContext->DispatchTable().destroyPipeline(mPipeline, nullptr);
            mPipeline = nullptr;
        }

        mContext         = context;
        mPipelineLayout = pipelineLayout;
        mShaderCollection.Clear();

        VkPhysicalDeviceRayTracingPipelinePropertiesKHR pipelineProperties{.sType = VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR};
        {
            VkPhysicalDeviceProperties2 prop2{};
            prop2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
            prop2.pNext = &pipelineProperties;
            vkGetPhysicalDeviceProperties2(mContext->VkPhysicalDevice(), &prop2);
        }


        /// STEP # 1    Rebuild shader collection

        mRaygenSbt.WriteToShaderCollection(mShaderCollection);
        mMissSbt.WriteToShaderCollection(mShaderCollection);
        mCallablesSbt.WriteToShaderCollection(mShaderCollection);
        mHitSbt.WriteToShaderCollection(mShaderCollection);


        /// STEP # 2    Collect ShaderGroup, build sorted vector

        std::vector<VkRayTracingShaderGroupCreateInfoKHR> shaderGroupCis;

        // Insert grouped into shaderGroupCis vector

        ShaderBindingTableBase::VectorRange raygenGroupHandleRange, missGroupHandleRange, callablesGroupHandleRange, hitGroupHandleRange;

        raygenGroupHandleRange    = mRaygenSbt.WriteToShaderGroupCiVector(shaderGroupCis, mShaderCollection);
        missGroupHandleRange      = mMissSbt.WriteToShaderGroupCiVector(shaderGroupCis, mShaderCollection);
        callablesGroupHandleRange = mCallablesSbt.WriteToShaderGroupCiVector(shaderGroupCis, mShaderCollection);
        hitGroupHandleRange       = mHitSbt.WriteToShaderGroupCiVector(shaderGroupCis, mShaderCollection);


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

        AssertVkResult(mContext->DispatchTable().createRayTracingPipelinesKHR(nullptr, nullptr, 1, &raytracingPipelineCreateInfo, nullptr, &mPipeline));


        /// STEP # 4    Get shader handles, build SBTs

        std::vector<uint8_t> shaderHandleData(shaderGroupCis.size() * pipelineProperties.shaderGroupHandleSize);
        AssertVkResult(mContext->DispatchTable().getRayTracingShaderGroupHandlesKHR(mPipeline, 0, shaderGroupCis.size(), shaderHandleData.size(), shaderHandleData.data()));

        {
            std::vector<const uint8_t*> handles(mRaygenSbt.GetGroups().size());
            for(int32_t i = 0; i < raygenGroupHandleRange.Count; i++)
            {
                size_t         handleOffset = (i + raygenGroupHandleRange.Start) * pipelineProperties.shaderGroupHandleSize;
                const uint8_t* handlePtr    = shaderHandleData.data() + handleOffset;
                handles[i]                  = handlePtr;
            }
            mRaygenSbt.Build(context, pipelineProperties, handles);
        }
        {
            std::vector<const uint8_t*> handles(mMissSbt.GetGroups().size());
            for(int32_t i = 0; i < missGroupHandleRange.Count; i++)
            {
                size_t         handleOffset = (i + missGroupHandleRange.Start) * pipelineProperties.shaderGroupHandleSize;
                const uint8_t* handlePtr    = shaderHandleData.data() + handleOffset;
                handles[i]                  = handlePtr;
            }
            mMissSbt.Build(context, pipelineProperties, handles);
        }
        {
            std::vector<const uint8_t*> handles(mCallablesSbt.GetGroups().size());
            for(int32_t i = 0; i < callablesGroupHandleRange.Count; i++)
            {
                size_t         handleOffset = (i + callablesGroupHandleRange.Start) * pipelineProperties.shaderGroupHandleSize;
                const uint8_t* handlePtr    = shaderHandleData.data() + handleOffset;
                handles[i]                  = handlePtr;
            }
            mCallablesSbt.Build(context, pipelineProperties, handles);
        }
        {
            std::vector<const uint8_t*> handles(mHitSbt.GetGroups().size());
            for(int32_t i = 0; i < hitGroupHandleRange.Count; i++)
            {
                size_t         handleOffset = (i + hitGroupHandleRange.Start) * pipelineProperties.shaderGroupHandleSize;
                const uint8_t* handlePtr    = shaderHandleData.data() + handleOffset;
                handles[i]                  = handlePtr;
            }
            mHitSbt.Build(context, pipelineProperties, handles);
        }
    }

    void RtPipeline::CmdBindPipeline(VkCommandBuffer cmdBuffer) const
    {
        vkCmdBindPipeline(cmdBuffer, VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, mPipeline);
    }

    void RtPipeline::Destroy()
    {
        mRaygenSbt.Destroy();
        mMissSbt.Destroy();
        mCallablesSbt.Destroy();
        mHitSbt.Destroy();
        if(!!mPipeline)
        {
            mContext->DispatchTable().destroyPipeline(mPipeline, nullptr);
            mPipeline = nullptr;
        }
    }
}  // namespace foray::rtpipe
