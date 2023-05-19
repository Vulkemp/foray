#include "defaultraytracingstage.hpp"
#include "../core/samplercollection.hpp"
#include "../core/shadermanager.hpp"
#include "../core/shadermodule.hpp"
#include "../scene/components/meshinstance.hpp"
#include "../scene/scene.hpp"
#include "../scene/globalcomponents/cameramanager.hpp"
#include "../scene/globalcomponents/geometrymanager.hpp"
#include "../scene/globalcomponents/materialmanager.hpp"
#include "../scene/globalcomponents/texturemanager.hpp"
#include "../scene/globalcomponents/tlasmanager.hpp"
#include "../util/pipelinebuilder.hpp"
#include "../util/shaderstagecreateinfos.hpp"
#include <array>

namespace foray::stages {
    DefaultRaytracingStageBase::DefaultRaytracingStageBase(core::Context* context, RenderDomain* domain, int32_t resizeOrder)
     : RenderStage(context, domain, resizeOrder)
    {

    }
    void DefaultRaytracingStageBase::Init(scene::Scene* scene, core::CombinedImageSampler* envMap, core::ManagedImage* noiseImage)
    {
        mScene          = scene;
        mEnvironmentMap = envMap;
        mNoiseTexture   = noiseImage;
        ApiCustomObjectsCreate();
        CreateOutputImages();
        CreateOrUpdateDescriptors();
        CreatePipelineLayout();
        ApiCreateRtPipeline();
    }
    void DefaultRaytracingStageBase::RecordFrame(VkCommandBuffer cmdBuffer, base::FrameRenderInfo& renderInfo)
    {
        RecordFramePrepare(cmdBuffer, renderInfo);
        RecordFrameBind(cmdBuffer, renderInfo);
        RecordFrameTraceRays(cmdBuffer, renderInfo);
    }
    void DefaultRaytracingStageBase::OnResized(VkExtent2D extent)
    {
        RenderStage::OnResized(extent);
        CreateOrUpdateDescriptors();
    }
    DefaultRaytracingStageBase::~DefaultRaytracingStageBase()
    {
    }
    void DefaultRaytracingStageBase::ReloadShaders()
    {
        ApiDestroyRtPipeline();
        ApiCreateRtPipeline();
    }
    void DefaultRaytracingStageBase::CreateOutputImages()
    {
        VkImageUsageFlags imageUsageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

        mOutput.New(mContext, imageUsageFlags, VK_FORMAT_R16G16B16A16_SFLOAT, mDomain->GetExtent(), OutputName);
        mImageOutputs[std::string(OutputName)] = mOutput.Get();
    }
    void DefaultRaytracingStageBase::CreatePipelineLayout()
    {
        util::PipelineLayout::Builder builder;
        builder.AddDescriptorSetLayout(mDescriptorSet.GetLayout());
        if(mRngSeedPushCOffset != ~0U)
        {
            builder.AddPushConstantRange<uint32_t>(RTSTAGEFLAGS, mRngSeedPushCOffset);
        }
        mPipelineLayout.New(mContext, builder);
    }
    void DefaultRaytracingStageBase::CreateOrUpdateDescriptors()
    {
        using namespace rtbindpoints;

        const as::Tlas&               tlas           = mScene->GetComponent<scene::gcomp::TlasManager>()->GetTlas();
        const as::GeometryMetaBuffer& metaBuffer     = tlas.GetMetaBuffer();
        auto                          materialBuffer = mScene->GetComponent<scene::gcomp::MaterialManager>();
        auto                          textureStore   = mScene->GetComponent<scene::gcomp::TextureManager>();
        auto                          cameraManager  = mScene->GetComponent<scene::gcomp::CameraManager>();
        auto                          geometryStore  = mScene->GetComponent<scene::gcomp::GeometryStore>();

        VkAccelerationStructureKHR accelStructure = tlas.GetAccelerationStructure();

        mDescriptorSet.SetDescriptorAt(BIND_TLAS, accelStructure, RTSTAGEFLAGS);
        mDescriptorSet.SetDescriptorAt(BIND_OUT_IMAGE, mOutput.Get(), VK_IMAGE_LAYOUT_GENERAL, nullptr, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, RTSTAGEFLAGS);
        mDescriptorSet.SetDescriptorAt(BIND_CAMERA_UBO, cameraManager->GetVkDescriptorInfo(), VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, RTSTAGEFLAGS);
        mDescriptorSet.SetDescriptorAt(BIND_VERTICES, geometryStore->GetVerticesBuffer().Get(), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, RTSTAGEFLAGS);
        mDescriptorSet.SetDescriptorAt(BIND_INDICES, geometryStore->GetIndicesBuffer().Get(), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, RTSTAGEFLAGS);
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

        mDescriptorSet.CreateOrUpdate(mContext, "RaytracingStageDescriptorSet");
    }
    void DefaultRaytracingStageBase::RecordFramePrepare(VkCommandBuffer cmdBuffer, base::FrameRenderInfo& renderInfo)
    {
        std::vector<VkImageMemoryBarrier2>  imageBarriers;
        std::vector<VkBufferMemoryBarrier2> bufferBarriers;

        {  // Image Memory Barriers
            imageBarriers.push_back(
                renderInfo.GetImageLayoutCache().MakeBarrier(mOutput.Get(), core::ImageLayoutCache::Barrier2{.SrcStageMask  = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
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
    void DefaultRaytracingStageBase::RecordFrameBind(VkCommandBuffer cmdBuffer, base::FrameRenderInfo& renderInfo)
    {
        mPipeline->CmdBindPipeline(cmdBuffer);

        VkDescriptorSet descriptorSet = mDescriptorSet.GetSet();

        vkCmdBindDescriptorSets(cmdBuffer, VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, mPipelineLayout.GetRef(), 0U, 1U, &descriptorSet, 0U, nullptr);
    }
    void DefaultRaytracingStageBase::RecordFrameTraceRays(VkCommandBuffer cmdBuffer, base::FrameRenderInfo& renderInfo)
    {
        if(mRngSeedPushCOffset != ~0U)
        {
            uint32_t pushC = renderInfo.GetFrameNumber();
            vkCmdPushConstants(cmdBuffer, mPipelineLayout.GetRef(), RTSTAGEFLAGS, mRngSeedPushCOffset, sizeof(pushC), &pushC);
        }

        mPipeline->CmdTraceRays(cmdBuffer, mOutput->GetExtent2D());
    }
}
