#include "foray_raytracingstage.hpp"
#include "../core/foray_samplercollection.hpp"
#include "../core/foray_shadermanager.hpp"
#include "../core/foray_shadermodule.hpp"
#include "../scene/components/foray_meshinstance.hpp"
#include "../scene/foray_scene.hpp"
#include "../scene/globalcomponents/foray_cameramanager.hpp"
#include "../scene/globalcomponents/foray_geometrystore.hpp"
#include "../scene/globalcomponents/foray_materialbuffer.hpp"
#include "../scene/globalcomponents/foray_texturestore.hpp"
#include "../scene/globalcomponents/foray_tlasmanager.hpp"
#include "../util/foray_pipelinebuilder.hpp"
#include "../util/foray_shaderstagecreateinfos.hpp"
#include <array>

namespace foray::stages {

    void RaytracingStage::Init()
    {
        CreateResolutionDependentComponents();
        CreateFixedSizeComponents();
    }

    void RaytracingStage::OnResized(const VkExtent2D& extent)
    {
        mRaytracingRenderTarget.Resize(VkExtent3D{extent.width, extent.height, 1});
        UpdateDescriptors();
    }

    void RaytracingStage::CreateFixedSizeComponents()
    {
        SetupDescriptors();
        CreateDescriptorSets();
        CreatePipelineLayout();
        CreateRaytraycingPipeline();
    }

    void RaytracingStage::DestroyFixedComponents()
    {
        DestroyShaders();
        mDescriptorSet.Destroy();
        mPipeline.Destroy();
        mPipelineLayout.Destroy();
    }

    void RaytracingStage::CreateResolutionDependentComponents()
    {
        CreateOutputImage();
    }

    void RaytracingStage::DestroyResolutionDependentComponents()
    {
        mImageOutputs.clear();
        mRaytracingRenderTarget.Destroy();
    }

    void RaytracingStage::CreateOutputImage()
    {
        static const VkFormat          colorFormat = VK_FORMAT_R32G32B32A32_SFLOAT;
        static const VkImageUsageFlags imageUsageFlags =
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;


        mRaytracingRenderTarget.Create(mContext, imageUsageFlags, colorFormat, mContext->GetSwapchainSize(), RaytracingRenderTargetName);
        core::ManagedImage::QuickTransition transition{.SrcStageMask = VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                                       .DstStageMask = VkPipelineStageFlagBits::VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                                                       .NewLayout    = VkImageLayout::VK_IMAGE_LAYOUT_GENERAL};
        mRaytracingRenderTarget.TransitionLayout(transition);

        mImageOutputs[std::string(RaytracingRenderTargetName)] = &mRaytracingRenderTarget;
    }

    void RaytracingStage::SetupDescriptors()
    {
        as::Tlas& tlas           = mScene->GetComponent<scene::gcomp::TlasManager>()->GetTlas();
        auto      materialBuffer = mScene->GetComponent<scene::gcomp::MaterialBuffer>();
        auto      textureStore   = mScene->GetComponent<scene::gcomp::TextureStore>();
        auto      cameraManager  = mScene->GetComponent<scene::gcomp::CameraManager>();
        auto      geometryStore  = mScene->GetComponent<scene::gcomp::GeometryStore>();


        mDescriptorAccelerationStructureInfo.sType                      = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
        mDescriptorAccelerationStructureInfo.accelerationStructureCount = 1;
        mDescriptorAccelerationStructureInfo.pAccelerationStructures    = &(tlas.GetAccelerationStructure());

        mDescriptorSet.SetDescriptorAt(0, &mDescriptorAccelerationStructureInfo, 1, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, RTSTAGEFLAGS);

        mDescriptorSet.SetDescriptorAt(1, mRaytracingRenderTarget, VK_IMAGE_LAYOUT_GENERAL, nullptr, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, RTSTAGEFLAGS);
        mDescriptorSet.SetDescriptorAt(2, cameraManager->GetVkDescriptorInfo(), VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, RTSTAGEFLAGS);

        mDescriptorSet.SetDescriptorAt(3, geometryStore->GetVerticesBuffer(), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, RTSTAGEFLAGS);

        mDescriptorSet.SetDescriptorAt(4, geometryStore->GetIndicesBuffer(), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, RTSTAGEFLAGS);

        mDescriptorSet.SetDescriptorAt(5, materialBuffer->GetVkDescriptorInfo(), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, RTSTAGEFLAGS);

        mDescriptorSet.SetDescriptorAt(6, textureStore->GetDescriptorInfos(), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, RTSTAGEFLAGS);
        as::GeometryMetaBuffer& metaBuffer = tlas.GetMetaBuffer();
        mDescriptorSet.SetDescriptorAt(7, metaBuffer.GetVkDescriptorInfo(), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, RTSTAGEFLAGS);

        if(!!mEnvMap)
        {
            mDescriptorSet.SetDescriptorAt(9, mEnvMap->GetVkDescriptorInfo(), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, RTSTAGEFLAGS);
        }
        if(!!mNoiseSource)
        {
            mDescriptorSet.SetDescriptorAt(10, mNoiseSource->GetVkDescriptorInfo(), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, RTSTAGEFLAGS);
        }
    }

    void RaytracingStage::CreateDescriptorSets()
    {
        mDescriptorSet.Create(mContext, "RaytraycingPipelineDescriptorSet");
    }

    void RaytracingStage::UpdateDescriptors()
    {
        VkShaderStageFlags rtShaderStages = VkShaderStageFlagBits::VK_SHADER_STAGE_RAYGEN_BIT_KHR | VkShaderStageFlagBits::VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR
                                            | VkShaderStageFlagBits::VK_SHADER_STAGE_MISS_BIT_KHR | VkShaderStageFlagBits::VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
        mDescriptorSet.SetDescriptorAt(1, mRaytracingRenderTarget, VK_IMAGE_LAYOUT_GENERAL, nullptr, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, rtShaderStages);
        mDescriptorSet.Update();
    }

    void RaytracingStage::CreatePipelineLayout()
    {
        mPipelineLayout.AddDescriptorSetLayout(mDescriptorSet.GetDescriptorSetLayout());
        mPipelineLayout.AddPushConstantRange<PushConstant>(RTSTAGEFLAGS);
        mPipelineLayout.Build(mContext);
    }

    void RaytracingStage::RecordFrame(VkCommandBuffer cmdBuffer, base::FrameRenderInfo& renderInfo)
    {
        std::vector<VkImageMemoryBarrier2> vkBarriers;
        {
            core::ImageLayoutCache::Barrier2 barrier{.SrcStageMask  = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
                                                     .SrcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT,
                                                     .DstStageMask  = VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR,
                                                     .DstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT,
                                                     .NewLayout     = VkImageLayout::VK_IMAGE_LAYOUT_GENERAL};
            vkBarriers.push_back(renderInfo.GetImageLayoutCache().Set(mRaytracingRenderTarget, barrier));
        }
        if (!!mNoiseSource)
        {
            core::ImageLayoutCache::Barrier2 barrier{.SrcStageMask  = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
                                                     .SrcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT,
                                                     .DstStageMask  = VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR,
                                                     .DstAccessMask = VK_ACCESS_2_SHADER_READ_BIT,
                                                     .NewLayout     = VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
            vkBarriers.push_back(renderInfo.GetImageLayoutCache().Set(mNoiseSource->GetManagedImage(), barrier));
        }

        VkDependencyInfo depInfo{
            .sType                   = VkStructureType::VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .dependencyFlags         = VkDependencyFlagBits::VK_DEPENDENCY_BY_REGION_BIT,
            .imageMemoryBarrierCount = (uint32_t)vkBarriers.size(),
            .pImageMemoryBarriers    = vkBarriers.data(),
        };

        vkCmdPipelineBarrier2(cmdBuffer, &depInfo);

        mPipeline.CmdBindPipeline(cmdBuffer);

        vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, mPipelineLayout, 0, 1, &mDescriptorSet.GetDescriptorSet(), 0, nullptr);

        mPushConstant.RngSeed = renderInfo.GetFrameNumber();
        vkCmdPushConstants(cmdBuffer, mPipelineLayout, RTSTAGEFLAGS, 0, sizeof(mPushConstant), &mPushConstant);

        VkStridedDeviceAddressRegionKHR raygen_shader_sbt_entry = mPipeline.GetRaygenSbt().GetAddressRegion();

        VkStridedDeviceAddressRegionKHR miss_shader_sbt_entry = mPipeline.GetMissSbt().GetAddressRegion();

        VkStridedDeviceAddressRegionKHR hit_shader_sbt_entry = mPipeline.GetHitSbt().GetAddressRegion();

        VkStridedDeviceAddressRegionKHR callable_shader_sbt_entry = mPipeline.GetCallablesSbt().GetAddressRegion();

        VkRect2D scissor{VkOffset2D{}, VkExtent2D{mContext->GetSwapchainSize().width, mContext->GetSwapchainSize().height}};
        mContext->VkbDispatchTable->cmdTraceRaysKHR(cmdBuffer, &raygen_shader_sbt_entry, &miss_shader_sbt_entry, &hit_shader_sbt_entry, &callable_shader_sbt_entry,
                                                    scissor.extent.width, scissor.extent.height, 1);
    }

    void RaytracingStage::ReloadShaders()
    {
        vkDeviceWaitIdle(mContext->Device());

        DestroyShaders();
        mPipeline.Destroy();
        CreateRaytraycingPipeline();
    }

    void RaytracingStage::CreateRaytraycingPipeline()
    {
        mPipeline.Build(mContext, mPipelineLayout);
    }
}  // namespace foray::stages