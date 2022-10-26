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
        mClearValues.resize(mColorAttachments.size() + 1);
        for(size_t i = 0; i < mColorAttachments.size(); i++)
        {
            mClearValues[i].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
        }
        mClearValues[mColorAttachments.size()].depthStencil = {1.0f, 0};
    }

    void RaytracingStage::OnResized(const VkExtent2D& extent)
    {
        RenderStage::OnResized(extent);
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
        mDescriptorSet.Destroy();
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
        PrepareAttachments();
    }

    void RaytracingStage::DestroyResolutionDependentComponents()
    {
        mColorAttachments.clear();
        mRaytracingRenderTarget.Destroy();
    }

    void RaytracingStage::PrepareAttachments()
    {
        static const VkFormat          colorFormat = VK_FORMAT_R32G32B32A32_SFLOAT;
        static const VkImageUsageFlags imageUsageFlags =
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

        VkExtent3D               extent                = {mContext->GetSwapchainSize().width, mContext->GetSwapchainSize().height, 1};
        VmaMemoryUsage           memoryUsage           = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
        VmaAllocationCreateFlags allocationCreateFlags = 0;
        VkImageLayout            intialLayout          = VK_IMAGE_LAYOUT_UNDEFINED;
        VkImageAspectFlags       aspectMask            = VK_IMAGE_ASPECT_COLOR_BIT;


        mRaytracingRenderTarget.Create(mContext, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, allocationCreateFlags, extent, imageUsageFlags, colorFormat, VK_IMAGE_LAYOUT_UNDEFINED,
                                       VK_IMAGE_ASPECT_COLOR_BIT, RaytracingRenderTargetName);
        core::ManagedImage::QuickTransition transition
        {
            .SrcStageMask = VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            .DstStageMask = VkPipelineStageFlagBits::VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
            .NewLayout = VkImageLayout::VK_IMAGE_LAYOUT_GENERAL
        };
        mRaytracingRenderTarget.TransitionLayout(transition);

        mColorAttachments = {&mRaytracingRenderTarget};
    }

    void RaytracingStage::SetupDescriptors()
    {
        VkShaderStageFlags rtShaderStages = VkShaderStageFlagBits::VK_SHADER_STAGE_RAYGEN_BIT_KHR | VkShaderStageFlagBits::VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR
                                            | VkShaderStageFlagBits::VK_SHADER_STAGE_MISS_BIT_KHR | VkShaderStageFlagBits::VK_SHADER_STAGE_ANY_HIT_BIT_KHR;

        mDescriptorSet.SetDescriptorInfoAt(0, GetAccelerationStructureDescriptorInfo());
        mDescriptorSet.SetDescriptorInfoAt(1, GetRenderTargetDescriptorInfo());
        mDescriptorSet.SetDescriptorInfoAt(2, mScene->GetComponent<scene::CameraManager>()->MakeUboDescriptorInfos(rtShaderStages));
        mDescriptorSet.SetDescriptorInfoAt(3, mScene->GetComponent<scene::GeometryStore>()->GetVertexBufferDescriptorInfo(rtShaderStages));
        mDescriptorSet.SetDescriptorInfoAt(4, mScene->GetComponent<scene::GeometryStore>()->GetIndexBufferDescriptorInfo(rtShaderStages));
        mDescriptorSet.SetDescriptorInfoAt(5, mScene->GetComponent<scene::MaterialBuffer>()->GetDescriptorInfo(rtShaderStages));
        mDescriptorSet.SetDescriptorInfoAt(6, mScene->GetComponent<scene::TextureStore>()->GetDescriptorInfo(rtShaderStages));
        mDescriptorSet.SetDescriptorInfoAt(7, mScene->GetComponent<scene::TlasManager>()->GetTlas().GetMetaBuffer().GetDescriptorInfo(rtShaderStages));
        if(mEnvMap.IsSet)
        {
            mDescriptorSet.SetDescriptorInfoAt(9, mEnvMap.GetDescriptorInfo());
        }
        if(mNoiseSource.IsSet)
        {
            mDescriptorSet.SetDescriptorInfoAt(10, mNoiseSource.GetDescriptorInfo());
        }
    }

    void RaytracingStage::CreateDescriptorSets()
    {
        mDescriptorSet.Create(mContext, "RaytraycingPipelineDescriptorSet");
    }

    void RaytracingStage::UpdateDescriptors()
    {
        mDescriptorSet.SetDescriptorInfoAt(1, GetRenderTargetDescriptorInfo(true));

        mDescriptorSet.Update(mContext);
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
        core::ImageLayoutCache::Barrier2 barrier{
            .SrcStageMask        = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
            .SrcAccessMask       = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT,
            .DstStageMask        = VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR,
            .DstAccessMask       = VK_ACCESS_2_MEMORY_WRITE_BIT,
            .NewLayout = VkImageLayout::VK_IMAGE_LAYOUT_GENERAL
        };
        VkImageMemoryBarrier2 vkBarrier = renderInfo.GetImageLayoutCache().Set(mRaytracingRenderTarget, barrier);

        VkDependencyInfo depInfo{
            .sType                   = VkStructureType::VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .dependencyFlags         = VkDependencyFlagBits::VK_DEPENDENCY_BY_REGION_BIT,
            .imageMemoryBarrierCount = 1U,
            .pImageMemoryBarriers    = &vkBarrier,
        };

        vkCmdPipelineBarrier2(cmdBuffer, &depInfo);

        mPipeline.CmdBindPipeline(cmdBuffer);

        const auto& descriptorsets = mDescriptorSet.GetDescriptorSets();
        vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, mPipelineLayout, 0, 1,
                                &(descriptorsets[(renderInfo.GetFrameNumber()) % descriptorsets.size()]), 0, nullptr);

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


    std::shared_ptr<core::DescriptorSetHelper::DescriptorInfo> RaytracingStage::GetAccelerationStructureDescriptorInfo(bool rebuild)
    {
        if(mAcclerationStructureDescriptorInfo != nullptr && !rebuild)
        {
            return mAcclerationStructureDescriptorInfo;
        }

        // Setup the descriptor for binding our top level acceleration structure to the ray tracing shaders
        mDescriptorAccelerationStructureInfo.sType                      = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
        mDescriptorAccelerationStructureInfo.accelerationStructureCount = 1;
        mDescriptorAccelerationStructureInfo.pAccelerationStructures    = &mScene->GetComponent<scene::TlasManager>()->GetTlas().GetAccelerationStructure();

        mAcclerationStructureDescriptorInfo = std::make_shared<core::DescriptorSetHelper::DescriptorInfo>();
        mAcclerationStructureDescriptorInfo->Init(VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
        mAcclerationStructureDescriptorInfo->AddPNext(&mDescriptorAccelerationStructureInfo, 1);
        return mAcclerationStructureDescriptorInfo;
    }

    std::shared_ptr<core::DescriptorSetHelper::DescriptorInfo> RaytracingStage::GetRenderTargetDescriptorInfo(bool rebuild)
    {
        if(mRenderTargetDescriptorInfo != nullptr && !rebuild)
        {
            return mRenderTargetDescriptorInfo;
        }

        UpdateRenderTargetDescriptorBufferInfos();

        mRenderTargetDescriptorInfo = std::make_shared<core::DescriptorSetHelper::DescriptorInfo>();
        mRenderTargetDescriptorInfo->Init(VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
                                          &mRenderTargetDescriptorImageInfos);
        return mRenderTargetDescriptorInfo;
    }

    std::shared_ptr<core::DescriptorSetHelper::DescriptorInfo> RaytracingStage::SampledImage::GetDescriptorInfo(bool rebuild)
    {
        if(!!DescriptorInfo && !rebuild)
        {
            return DescriptorInfo;
        }

        UpdateDescriptorInfos();

        DescriptorInfo = std::make_shared<core::DescriptorSetHelper::DescriptorInfo>();
        DescriptorInfo->Init(VkDescriptorType::VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, RTSTAGEFLAGS, &DescriptorImageInfos);
        return DescriptorInfo;
    }

    void RaytracingStage::UpdateRenderTargetDescriptorBufferInfos()
    {
        mRenderTargetDescriptorImageInfos.resize(1);
        mRenderTargetDescriptorImageInfos[0].imageView   = mRaytracingRenderTarget.GetImageView();
        mRenderTargetDescriptorImageInfos[0].imageLayout = VkImageLayout::VK_IMAGE_LAYOUT_GENERAL;
    }

    void RaytracingStage::SampledImage::UpdateDescriptorInfos()
    {
        DescriptorImageInfos = {VkDescriptorImageInfo{.sampler = Sampler, .imageView = Image->GetImageView(), .imageLayout = VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}};  // namespace foray
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
        DescriptorImageInfos.clear();
        DescriptorInfo = nullptr;
    }

}  // namespace foray::stages