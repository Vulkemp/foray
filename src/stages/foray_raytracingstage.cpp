#include "foray_raytracingstage.hpp"
#include "../core/foray_samplercollection.hpp"
#include "../core/foray_shadermanager.hpp"
#include "../core/foray_shadermodule.hpp"
#include "../scene/components/foray_meshinstance.hpp"
#include "../scene/foray_scene.hpp"
#include "../scene/globalcomponents/foray_cameramanager.hpp"
#include "../scene/globalcomponents/foray_geometrymanager.hpp"
#include "../scene/globalcomponents/foray_materialmanager.hpp"
#include "../scene/globalcomponents/foray_texturemanager.hpp"
#include "../scene/globalcomponents/foray_tlasmanager.hpp"
#include "../util/foray_pipelinebuilder.hpp"
#include "../util/foray_shaderstagecreateinfos.hpp"
#include <array>

namespace foray::stages {

    void BasicRaytracingStage::Init(core::Context* context)
    {
        Destroy();
        mContext = context;
        CustomObjectsCreate();
        CreateOutputImages();
        CreateOrUpdateDescriptors();
        CreatePipelineLayout();
        CreateRtPipeline();
    }
    void BasicRaytracingStage::RecordFrame(VkCommandBuffer cmdBuffer, base::FrameRenderInfo& renderInfo)
    {
        RecordFramePrepare(cmdBuffer, renderInfo);
        RecordFrameBind(cmdBuffer, renderInfo);
        RecordFrameTraceRays(cmdBuffer, renderInfo);
    }
    void BasicRaytracingStage::Resize(const VkExtent2D& extent)
    {
        RenderStage::Resize(extent);
        CreateOrUpdateDescriptors();
    }
    void BasicRaytracingStage::Destroy()
    {
        DestroyRtPipeline();
        mPipelineLayout.Destroy();
        DestroyDescriptors();
        DestroyOutputImages();
        CustomObjectsDestroy();
    }
    void BasicRaytracingStage::ReloadShaders()
    {
        DestroyRtPipeline();
        CreateRtPipeline();
    }

    void ExtRaytracingStage::Init(core::Context* context, scene::Scene* scene, core::CombinedImageSampler* envMap, core::ManagedImage* noiseImage)
    {
        Destroy();
        mScene          = scene;
        mEnvironmentMap = envMap;
        mNoiseTexture   = noiseImage;
        BasicRaytracingStage::Init(context);
    }
    void ExtRaytracingStage::CreateOutputImages()
    {
        VkImageUsageFlags imageUsageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

        mOutput.Create(mContext, imageUsageFlags, VK_FORMAT_R16G16B16A16_SFLOAT, mContext->GetSwapchainSize(), OutputName);
        mImageOutputs[std::string(OutputName)] = &mOutput;
    }
    void ExtRaytracingStage::CreatePipelineLayout()
    {
        mPipelineLayout.AddDescriptorSetLayout(mDescriptorSet.GetDescriptorSetLayout());
        if(mRngSeedPushCOffset != ~0U)
        {
            mPipelineLayout.AddPushConstantRange<uint32_t>(RTSTAGEFLAGS, mRngSeedPushCOffset);
        }
        mPipelineLayout.Build(mContext);
    }
    void ExtRaytracingStage::CreateOrUpdateDescriptors()
    {
        using namespace rtbindpoints;

        const as::Tlas&               tlas           = mScene->GetComponent<scene::gcomp::TlasManager>()->GetTlas();
        const as::GeometryMetaBuffer& metaBuffer     = tlas.GetMetaBuffer();
        auto                          materialBuffer = mScene->GetComponent<scene::gcomp::MaterialManager>();
        auto                          textureStore   = mScene->GetComponent<scene::gcomp::TextureManager>();
        auto                          cameraManager  = mScene->GetComponent<scene::gcomp::CameraManager>();
        auto                          geometryStore  = mScene->GetComponent<scene::gcomp::GeometryStore>();

        VkAccelerationStructureKHR accelStructure = tlas.GetAccelerationStructure();

        mDescriptorAccelerationStructureInfo.sType                      = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
        mDescriptorAccelerationStructureInfo.accelerationStructureCount = 1;
        mDescriptorAccelerationStructureInfo.pAccelerationStructures    = &accelStructure;

        mDescriptorSet.SetDescriptorAt(BIND_TLAS, &mDescriptorAccelerationStructureInfo, 1, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, RTSTAGEFLAGS);
        mDescriptorSet.SetDescriptorAt(BIND_OUT_IMAGE, mOutput, VK_IMAGE_LAYOUT_GENERAL, nullptr, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, RTSTAGEFLAGS);
        mDescriptorSet.SetDescriptorAt(BIND_CAMERA_UBO, cameraManager->GetVkDescriptorInfo(), VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, RTSTAGEFLAGS);
        mDescriptorSet.SetDescriptorAt(BIND_VERTICES, geometryStore->GetVerticesBuffer(), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, RTSTAGEFLAGS);
        mDescriptorSet.SetDescriptorAt(BIND_INDICES, geometryStore->GetIndicesBuffer(), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, RTSTAGEFLAGS);
        mDescriptorSet.SetDescriptorAt(BIND_MATERIAL_BUFFER, materialBuffer->GetVkDescriptorInfo(), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, RTSTAGEFLAGS);
        mDescriptorSet.SetDescriptorAt(BIND_TEXTURES_ARRAY, textureStore->GetDescriptorInfos(), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, RTSTAGEFLAGS);
        mDescriptorSet.SetDescriptorAt(BIND_GEOMETRYMETA, metaBuffer.GetVkDescriptorInfo(), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, RTSTAGEFLAGS);

        if(!!mEnvironmentMap)
        {
            mDescriptorSet.SetDescriptorAt(BIND_ENVMAP_SPHERESAMPLER, mEnvironmentMap->GetVkDescriptorInfo(), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, RTSTAGEFLAGS);
        }
        if(!!mNoiseTexture)
        {
            mDescriptorSet.SetDescriptorAt(BIND_NOISETEX, mNoiseTexture, VkImageLayout::VK_IMAGE_LAYOUT_GENERAL, nullptr, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, RTSTAGEFLAGS);
        }

        if(mDescriptorSet.Exists())
        {
            mDescriptorSet.Update();
        }
        else
        {
            mDescriptorSet.Create(mContext, "RaytracingStageDescriptorSet");
        }
    }
    void ExtRaytracingStage::DestroyDescriptors()
    {
        mDescriptorSet.Destroy();
    }
    void ExtRaytracingStage::RecordFramePrepare(VkCommandBuffer cmdBuffer, base::FrameRenderInfo& renderInfo)
    {
        std::vector<VkImageMemoryBarrier2>  imageBarriers;
        std::vector<VkBufferMemoryBarrier2> bufferBarriers;

        {  // Image Memory Barriers
            imageBarriers.push_back(
                renderInfo.GetImageLayoutCache().MakeBarrier(mOutput, core::ImageLayoutCache::Barrier2{.SrcStageMask  = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
                                                                                                       .SrcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT,
                                                                                                       .DstStageMask  = VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR,
                                                                                                       .DstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT,
                                                                                                       .NewLayout     = VkImageLayout::VK_IMAGE_LAYOUT_GENERAL}));
        }
        {
            auto cameraManager = mScene->GetComponent<scene::gcomp::CameraManager>();

            bufferBarriers.push_back(cameraManager->GetUbo().MakeBarrierPrepareForRead(VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR, VK_ACCESS_SHADER_READ_BIT));
        }

        VkDependencyInfo depInfo{
            .sType                    = VkStructureType::VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .dependencyFlags          = VkDependencyFlagBits::VK_DEPENDENCY_BY_REGION_BIT,
            .bufferMemoryBarrierCount = (uint32_t)bufferBarriers.size(),
            .pBufferMemoryBarriers    = bufferBarriers.data(),
            .imageMemoryBarrierCount  = (uint32_t)imageBarriers.size(),
            .pImageMemoryBarriers     = imageBarriers.data(),
        };

        vkCmdPipelineBarrier2(cmdBuffer, &depInfo);
    }
    void ExtRaytracingStage::RecordFrameBind(VkCommandBuffer cmdBuffer, base::FrameRenderInfo& renderInfo)
    {
        mPipeline.CmdBindPipeline(cmdBuffer);

        VkDescriptorSet descriptorSet = mDescriptorSet.GetDescriptorSet();

        vkCmdBindDescriptorSets(cmdBuffer, VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, mPipelineLayout, 0U, 1U, &descriptorSet, 0U, nullptr);
    }
    void ExtRaytracingStage::RecordFrameTraceRays(VkCommandBuffer cmdBuffer, base::FrameRenderInfo& renderInfo)
    {
        if(mRngSeedPushCOffset != ~0U)
        {
            uint32_t pushC = renderInfo.GetFrameNumber();
            vkCmdPushConstants(cmdBuffer, mPipelineLayout, RTSTAGEFLAGS, mRngSeedPushCOffset, sizeof(pushC), &pushC);
        }

        VkStridedDeviceAddressRegionKHR raygen_shader_sbt_entry = mPipeline.GetRaygenSbt().GetAddressRegion();

        VkStridedDeviceAddressRegionKHR miss_shader_sbt_entry = mPipeline.GetMissSbt().GetAddressRegion();

        VkStridedDeviceAddressRegionKHR hit_shader_sbt_entry = mPipeline.GetHitSbt().GetAddressRegion();

        VkStridedDeviceAddressRegionKHR callable_shader_sbt_entry = mPipeline.GetCallablesSbt().GetAddressRegion();

        VkExtent2D extent{mOutput.GetExtent3D().width, mOutput.GetExtent3D().height};

        mContext->VkbDispatchTable->cmdTraceRaysKHR(cmdBuffer, &raygen_shader_sbt_entry, &miss_shader_sbt_entry, &hit_shader_sbt_entry, &callable_shader_sbt_entry, extent.width,
                                                    extent.height, 1U);
    }
}  // namespace foray::stages