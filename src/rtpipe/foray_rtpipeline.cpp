#include "foray_rtpipeline.hpp"
#include <array>

namespace foray::rtpipe {

    RtPipeline::RtPipeline(core::Context* context, const Builder& builderConst) : mContext(context), mPipelineLayout(builderConst.GetPipelineLayout())
    {
        /// STEP # 0    Reset, get physical device properties

        Builder builder = builderConst;  // Need a copy to be able to assure correct SBT sizing

        {
            std::array<GeneralShaderBindingTable::Builder*, 3> generalSbtBuilders({&builder.GetRaygenSbtBuilder(), &builder.GetMissSbtBuilder(), &builder.GetCallableSbtBuilder()});
            for(GeneralShaderBindingTable::Builder* builder : generalSbtBuilders)
            {
                if(builder->GetModules().size() > builder->GetGroupData().size())
                {
                    builder->GetGroupData().resize(builder->GetModules().size());
                }
            }
            HitShaderBindingTable::Builder& hitSbtBuilder = builder.GetHitSbtBuilder();

            if(hitSbtBuilder.GetModules().size() > hitSbtBuilder.GetGroupData().size())
            {
                hitSbtBuilder.GetGroupData().resize(hitSbtBuilder.GetModules().size());
            }
        }

        VkPhysicalDeviceRayTracingPipelinePropertiesKHR pipelineProperties{.sType = VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR};
        {
            VkPhysicalDeviceProperties2 prop2{};
            prop2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
            prop2.pNext = &pipelineProperties;
            vkGetPhysicalDeviceProperties2(mContext->VkPhysicalDevice(), &prop2);
        }


        /// STEP # 1    Build shader collection

        RtShaderCollection shaderCollection;

        builder.GetRaygenSbtBuilder().WriteToShaderCollection(shaderCollection);
        builder.GetMissSbtBuilder().WriteToShaderCollection(shaderCollection);
        builder.GetCallableSbtBuilder().WriteToShaderCollection(shaderCollection);
        builder.GetHitSbtBuilder().WriteToShaderCollection(shaderCollection);


        /// STEP # 2    Collect ShaderGroup, build sorted vector

        std::vector<VkRayTracingShaderGroupCreateInfoKHR> shaderGroupCis;

        // Insert grouped into shaderGroupCis vector

        ShaderBindingTableBase::VectorRange raygenGroupHandleRange, missGroupHandleRange, callablesGroupHandleRange, hitGroupHandleRange;

        raygenGroupHandleRange    = builder.GetRaygenSbtBuilder().WriteToShaderGroupCiVector(shaderGroupCis, shaderCollection);
        missGroupHandleRange      = builder.GetMissSbtBuilder().WriteToShaderGroupCiVector(shaderGroupCis, shaderCollection);
        callablesGroupHandleRange = builder.GetCallableSbtBuilder().WriteToShaderGroupCiVector(shaderGroupCis, shaderCollection);
        hitGroupHandleRange       = builder.GetHitSbtBuilder().WriteToShaderGroupCiVector(shaderGroupCis, shaderCollection);


        /// STEP # 3    Build RT pipeline

        // Create the ray tracing pipeline
        VkRayTracingPipelineCreateInfoKHR raytracingPipelineCreateInfo{
            .sType                        = VkStructureType::VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR,
            .stageCount                   = static_cast<uint32_t>(shaderCollection.GetShaderStageCis().size()),
            .pStages                      = shaderCollection.GetShaderStageCis().data(),
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
            GeneralShaderBindingTable::Builder& sbtBuilder = builder.GetRaygenSbtBuilder();
            std::vector<const uint8_t*> handles(sbtBuilder.GetModules().size());
            for(int32_t i = 0; i < raygenGroupHandleRange.Count; i++)
            {
                size_t         handleOffset = (i + raygenGroupHandleRange.Start) * pipelineProperties.shaderGroupHandleSize;
                const uint8_t* handlePtr    = shaderHandleData.data() + handleOffset;
                handles[i]                  = handlePtr;
            }
            sbtBuilder.SetSgHandles(&handles).SetPipelineProperties(&pipelineProperties);
            mRaygenSbt.New(context, sbtBuilder);
        }
        {
            GeneralShaderBindingTable::Builder& sbtBuilder = builder.GetMissSbtBuilder();
            std::vector<const uint8_t*> handles(sbtBuilder.GetModules().size());
            for(int32_t i = 0; i < missGroupHandleRange.Count; i++)
            {
                size_t         handleOffset = (i + missGroupHandleRange.Start) * pipelineProperties.shaderGroupHandleSize;
                const uint8_t* handlePtr    = shaderHandleData.data() + handleOffset;
                handles[i]                  = handlePtr;
            }
            sbtBuilder.SetSgHandles(&handles).SetPipelineProperties(&pipelineProperties);
            mMissSbt.New(context, sbtBuilder);
        }
        {
            GeneralShaderBindingTable::Builder& sbtBuilder = builder.GetCallableSbtBuilder();
            std::vector<const uint8_t*> handles(sbtBuilder.GetModules().size());
            for(int32_t i = 0; i < callablesGroupHandleRange.Count; i++)
            {
                size_t         handleOffset = (i + callablesGroupHandleRange.Start) * pipelineProperties.shaderGroupHandleSize;
                const uint8_t* handlePtr    = shaderHandleData.data() + handleOffset;
                handles[i]                  = handlePtr;
            }
            sbtBuilder.SetSgHandles(&handles).SetPipelineProperties(&pipelineProperties);
            mCallablesSbt.New(context, sbtBuilder);
        }
        {
            HitShaderBindingTable::Builder& sbtBuilder = builder.GetHitSbtBuilder();
            std::vector<const uint8_t*> handles(sbtBuilder.GetModules().size());
            for(int32_t i = 0; i < hitGroupHandleRange.Count; i++)
            {
                size_t         handleOffset = (i + hitGroupHandleRange.Start) * pipelineProperties.shaderGroupHandleSize;
                const uint8_t* handlePtr    = shaderHandleData.data() + handleOffset;
                handles[i]                  = handlePtr;
            }
            sbtBuilder.SetSgHandles(&handles).SetPipelineProperties(&pipelineProperties);
            mHitSbt.New(context, sbtBuilder);
        }
    }

    void RtPipeline::CmdBindPipeline(VkCommandBuffer cmdBuffer) const
    {
        vkCmdBindPipeline(cmdBuffer, VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, mPipeline);
    }

    void RtPipeline::CmdTraceRays(VkCommandBuffer cmdBuffer, VkExtent3D launchSize) const
    {
        VkStridedDeviceAddressRegionKHR raygenSbtRegion = mRaygenSbt->GetAddressRegion();
        VkStridedDeviceAddressRegionKHR missSbtRegion = mMissSbt->GetAddressRegion();
        VkStridedDeviceAddressRegionKHR hitSbtRegion = mHitSbt->GetAddressRegion();
        VkStridedDeviceAddressRegionKHR callableSbtRegion = mCallablesSbt->GetAddressRegion();

        mContext->DispatchTable().cmdTraceRaysKHR(cmdBuffer, &raygenSbtRegion, &missSbtRegion, &hitSbtRegion, &callableSbtRegion, launchSize.width, launchSize.height, launchSize.depth);
    }

    RtPipeline::~RtPipeline()
    {
        if(!!mPipeline)
        {
            mContext->DispatchTable().destroyPipeline(mPipeline, nullptr);
            mPipeline = nullptr;
        }
    }
}  // namespace foray::rtpipe
