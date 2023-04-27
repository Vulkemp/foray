#include "foray_raytracingstage.hpp"
#include "../core/foray_samplercollection.hpp"
#include "../scene/components/foray_meshinstance.hpp"
#include "../scene/foray_scene.hpp"
#include "../scene/globalcomponents/foray_cameramanager.hpp"
#include "../scene/globalcomponents/foray_geometrymanager.hpp"
#include "../scene/globalcomponents/foray_materialmanager.hpp"
#include "../scene/globalcomponents/foray_texturemanager.hpp"
#include "../scene/globalcomponents/foray_tlasmanager.hpp"
#include "../util/foray_shaderstagecreateinfos.hpp"

namespace foray::stages {
    void RaytracingStageBase::Init(core::Context* context, scene::Scene* scene, core::CombinedImageSampler* envMap, core::ManagedImage* noiseImage)
    {
        Destroy();
        mScene          = scene;
        mEnvironmentMap = envMap;
        mNoiseTexture   = noiseImage;
        mContext = context;
        ApiCustomObjectsCreate();
        CreateOutputImages();
        CreateOrUpdateDescriptors();
        CreatePipelineLayout();
        ApiCreateRtPipeline();
    }
    void RaytracingStageBase::RecordFrame(VkCommandBuffer cmdBuffer, base::FrameRenderInfo& renderInfo)
    {
        // barriers
        {
            std::vector<VkImageMemoryBarrier2> imageBarriers;
            std::vector<VkBufferMemoryBarrier2> bufferBarriers;
            RecordFrameBarriers(cmdBuffer, renderInfo, imageBarriers, bufferBarriers);
            VkDependencyInfo depInfo{
                    .sType                    = VkStructureType::VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                    .dependencyFlags          = VkDependencyFlagBits::VK_DEPENDENCY_BY_REGION_BIT,
                    .bufferMemoryBarrierCount = (uint32_t) bufferBarriers.size(),
                    .pBufferMemoryBarriers    = bufferBarriers.data(),
                    .imageMemoryBarrierCount  = (uint32_t) imageBarriers.size(),
                    .pImageMemoryBarriers     = imageBarriers.data(),
            };
            vkCmdPipelineBarrier2(cmdBuffer, &depInfo);
        }

        RecordFrameBind(cmdBuffer, renderInfo);
        RecordFrameTraceRays(cmdBuffer, renderInfo);
    }
    void RaytracingStageBase::Resize(const VkExtent2D& extent)
    {
        RenderStage::Resize(extent);
        CreateOrUpdateDescriptors();
    }
    void RaytracingStageBase::Destroy()
    {
        ApiDestroyRtPipeline();
        mPipelineLayout.Destroy();
        DestroyDescriptors();
        DestroyOutputImages();
        ApiCustomObjectsDestroy();
    }
    void RaytracingStageBase::ReloadShaders()
    {
        ApiDestroyRtPipeline();
        ApiCreateRtPipeline();
    }
    void RaytracingStageBase::CreatePipelineLayout()
    {
        mPipelineLayout.AddDescriptorSetLayout(mDescriptorSet.GetDescriptorSetLayout());
        if(mRngSeedPushCOffset != ~0U)
        {
            mPipelineLayout.AddPushConstantRange<uint32_t>(RTSTAGEFLAGS, mRngSeedPushCOffset);
        }
        mPipelineLayout.Build(mContext);
    }
    void RaytracingStageBase::CreateOrUpdateDescriptors()
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
    void RaytracingStageBase::DestroyDescriptors()
    {
        mDescriptorSet.Destroy();
    }
    void RaytracingStageBase::RecordFrameBarriers(VkCommandBuffer cmdBuffer, base::FrameRenderInfo &renderInfo, std::vector<VkImageMemoryBarrier2> &imageBarriers,
                                                  std::vector<VkBufferMemoryBarrier2> &bufferBarriers) {
        auto cameraManager = mScene->GetComponent<scene::gcomp::CameraManager>();
        bufferBarriers.push_back(cameraManager->GetUbo().MakeBarrierPrepareForRead(VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR, VK_ACCESS_SHADER_READ_BIT));
    }
    void RaytracingStageBase::RecordFrameBind(VkCommandBuffer cmdBuffer, base::FrameRenderInfo& renderInfo)
    {
        mPipeline.CmdBindPipeline(cmdBuffer);

        VkDescriptorSet descriptorSet = mDescriptorSet.GetDescriptorSet();
        vkCmdBindDescriptorSets(cmdBuffer, VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, mPipelineLayout, 0U, 1U, &descriptorSet, 0U, nullptr);

        if(mRngSeedPushCOffset != ~0U)
        {
            uint32_t pushC = renderInfo.GetFrameNumber();
            vkCmdPushConstants(cmdBuffer, mPipelineLayout, RTSTAGEFLAGS, mRngSeedPushCOffset, sizeof(pushC), &pushC);
        }
    }
}
