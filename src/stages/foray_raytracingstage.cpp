#include "foray_raytracingstage.hpp"
#include "../core/foray_shadermanager.hpp"
#include "../core/foray_shadermodule.hpp"
#include "../scene/components/foray_meshinstance.hpp"
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

        // Clear values for all attachments written in the fragment shader
        mClearValues.resize(mImageOutputs.size() + 1);
        for(size_t i = 0; i < mImageOutputs.size(); i++)
        {
            mClearValues[i].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
        }
        mClearValues[mImageOutputs.size()].depthStencil = {1.0f, 0};
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
        if(!!mContext)
        {
            VkDevice device = mContext->Device();
            if(!!mPipelineLayout)
            {
                vkDestroyPipelineLayout(device, mPipelineLayout, nullptr);
                mPipelineLayout = nullptr;
            }
        }
        if(mEnvMap.IsSet)
        {
            mEnvMap.Destroy(mContext);
        }
        if(mNoiseSource.IsSet)
        {
            mNoiseSource.Destroy(mContext);
        }
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

        VkExtent3D               extent                = {mContext->GetSwapchainSize().width, mContext->GetSwapchainSize().height, 1};
        VmaMemoryUsage           memoryUsage           = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
        VmaAllocationCreateFlags allocationCreateFlags = 0;
        VkImageAspectFlags       aspectMask            = VK_IMAGE_ASPECT_COLOR_BIT;


        mRaytracingRenderTarget.Create(mContext, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, allocationCreateFlags, extent, imageUsageFlags, colorFormat,
                                       VK_IMAGE_ASPECT_COLOR_BIT, RaytracingRenderTargetName);
        core::ManagedImage::QuickTransition transition{.SrcStageMask = VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                                       .DstStageMask = VkPipelineStageFlagBits::VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                                                       .NewLayout    = VkImageLayout::VK_IMAGE_LAYOUT_GENERAL};
        mRaytracingRenderTarget.TransitionLayout(transition);

        mImageOutputs[std::string(RaytracingRenderTargetName)] = &mRaytracingRenderTarget;
    }

    void RaytracingStage::SetupDescriptors()
    {
        as::Tlas& tlas = mScene->GetComponent<scene::TlasManager>()->GetTlas();

        mDescriptorAccelerationStructureInfo.sType                      = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
        mDescriptorAccelerationStructureInfo.accelerationStructureCount = 1;
        mDescriptorAccelerationStructureInfo.pAccelerationStructures    = &(tlas.GetAccelerationStructure());

        mDescriptorSet.SetDescriptorAt(0, &mDescriptorAccelerationStructureInfo, 1, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, RTSTAGEFLAGS);

        mDescriptorSet.SetDescriptorAt(1, mRaytracingRenderTarget, VK_IMAGE_LAYOUT_GENERAL, nullptr, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, RTSTAGEFLAGS);
        mDescriptorSet.SetDescriptorAt(2, mScene->GetComponent<scene::CameraManager>()->GetVkDescriptorInfo(), VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                       RTSTAGEFLAGS);

        mDescriptorSet.SetDescriptorAt(3, mScene->GetComponent<scene::GeometryStore>()->GetVerticesBuffer(), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, RTSTAGEFLAGS);

        mDescriptorSet.SetDescriptorAt(4, mScene->GetComponent<scene::GeometryStore>()->GetIndicesBuffer(), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, RTSTAGEFLAGS);

        mDescriptorSet.SetDescriptorAt(5, mScene->GetComponent<scene::MaterialBuffer>()->GetVkDescriptorInfo(), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, RTSTAGEFLAGS);

        mDescriptorSet.SetDescriptorAt(6, mScene->GetComponent<scene::TextureStore>()->GetDescriptorInfos(), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, RTSTAGEFLAGS);
        as::GeometryMetaBuffer& metaBuffer = tlas.GetMetaBuffer();
        mDescriptorSet.SetDescriptorAt(7, metaBuffer.GetVkDescriptorInfo(), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, RTSTAGEFLAGS);

        if(mEnvMap.IsSet)
        {
            mDescriptorSet.SetDescriptorAt(9, mEnvMap.Image, VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, mEnvMap.Sampler, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, RTSTAGEFLAGS);
        }
        if(mNoiseSource.IsSet)
        {
            mDescriptorSet.SetDescriptorAt(10, mNoiseSource.Image, VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, mNoiseSource.Sampler, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, RTSTAGEFLAGS);
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
        VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
        pipelineLayoutCreateInfo.sType          = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutCreateInfo.setLayoutCount = 1;
        pipelineLayoutCreateInfo.pSetLayouts    = &mDescriptorSet.GetDescriptorSetLayout();
        VkPushConstantRange pushC{
            .stageFlags = VkShaderStageFlagBits::VK_SHADER_STAGE_RAYGEN_BIT_KHR | VkShaderStageFlagBits::VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
            .offset     = 0,
            .size       = sizeof(mPushConstant),
        };
        pipelineLayoutCreateInfo.pPushConstantRanges    = &pushC;
        pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
        AssertVkResult(vkCreatePipelineLayout(mContext->Device(), &pipelineLayoutCreateInfo, nullptr, &mPipelineLayout));
    }

    void RaytracingStage::RecordFrame(VkCommandBuffer cmdBuffer, base::FrameRenderInfo& renderInfo)
    {
        core::ImageLayoutCache::Barrier2 barrier{.SrcStageMask  = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
                                                 .SrcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT,
                                                 .DstStageMask  = VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR,
                                                 .DstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT,
                                                 .NewLayout     = VkImageLayout::VK_IMAGE_LAYOUT_GENERAL};
        VkImageMemoryBarrier2            vkBarrier = renderInfo.GetImageLayoutCache().Set(mRaytracingRenderTarget, barrier);

        VkDependencyInfo depInfo{
            .sType                   = VkStructureType::VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .dependencyFlags         = VkDependencyFlagBits::VK_DEPENDENCY_BY_REGION_BIT,
            .imageMemoryBarrierCount = 1U,
            .pImageMemoryBarriers    = &vkBarrier,
        };

        vkCmdPipelineBarrier2(cmdBuffer, &depInfo);

        mPipeline.CmdBindPipeline(cmdBuffer);

        vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, mPipelineLayout, 0, 1, &mDescriptorSet.GetDescriptorSet(), 0, nullptr);

        mPushConstant.RngSeed = renderInfo.GetFrameNumber();
        vkCmdPushConstants(cmdBuffer, mPipelineLayout, VkShaderStageFlagBits::VK_SHADER_STAGE_RAYGEN_BIT_KHR | VkShaderStageFlagBits::VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, 0,
                           sizeof(mPushConstant), &mPushConstant);

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


    void RaytracingStage::SampledImage::Create(core::Context* context, core::ManagedImage* image, bool initateSampler)
    {
        Image = image;
        if(!!Image && initateSampler)
        {
            if(!!Sampler)
            {
                vkDestroySampler(context->Device(), Sampler, nullptr);
            }
            VkSamplerCreateInfo samplerCi{.sType                   = VkStructureType::VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
                                          .magFilter               = VkFilter::VK_FILTER_LINEAR,
                                          .minFilter               = VkFilter::VK_FILTER_LINEAR,
                                          .addressModeU            = VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_REPEAT,
                                          .addressModeV            = VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_REPEAT,
                                          .addressModeW            = VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_REPEAT,
                                          .anisotropyEnable        = VK_FALSE,
                                          .compareEnable           = VK_FALSE,
                                          .minLod                  = 0,
                                          .maxLod                  = 0,
                                          .unnormalizedCoordinates = VK_FALSE};
            AssertVkResult(vkCreateSampler(context->Device(), &samplerCi, nullptr, &Sampler));
        }
        IsSet = !!Image;
    }

    void RaytracingStage::SampledImage::Destroy(core::Context* context)
    {
        if(!!Sampler)
        {
            vkDestroySampler(context->Device(), Sampler, nullptr);
            Sampler = nullptr;
        }
        Image = nullptr;
        IsSet = false;
    }

}  // namespace foray::stages