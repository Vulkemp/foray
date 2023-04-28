#include "foray_comparerstage.hpp"
#include "../osi/foray_inputdevice.hpp"
#include "../osi/foray_osi_event.hpp"
#include "../osi/foray_osmanager.hpp"

const uint32_t SHADER_F[] =
#include "foray_comparerstage.f.comp.spv.h"
    ;
const uint32_t SHADER_I[] =
#include "foray_comparerstage.i.comp.spv.h"
    ;
const uint32_t SHADER_U[] =
#include "foray_comparerstage.u.comp.spv.h"
    ;

namespace foray::stages {
#pragma region Init
    ComparerStage::ComparerStage(core::Context* context, RenderDomain* domain, bool flipY, int32_t resizeOrder)
    {
        RenderStage::InitCallbacks(context, domain, resizeOrder);
        mOnMouseMoved.Set(mContext->OsManager->OnEventInputMouseMoved(), [this](const osi::EventInputMouseMoved* event) { this->HandleMouseMovedEvent(event); });
        mFlipY = flipY;
        CreateOutputImage();
        LoadShaders();
        CreatePipetteBuffer();
    }

    void ComparerStage::CreateOutputImage()
    {
        VkImageUsageFlags usage =
            VkImageUsageFlagBits::VK_IMAGE_USAGE_STORAGE_BIT | VkImageUsageFlagBits::VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VkImageUsageFlagBits::VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        core::ManagedImage::CreateInfo ci(usage, VkFormat::VK_FORMAT_R32G32B32A32_SFLOAT, mDomain->GetExtent(), OutputName);
        mOutput.New(mContext, ci);
        mImageOutputs[std::string(mOutput->GetName())] = mOutput.Get();
    }
    void ComparerStage::LoadShaders()
    {
        mShaders[(size_t)EInputType::Float].New();
        mShaders[(size_t)EInputType::Int].New();
        mShaders[(size_t)EInputType::Uint].New();
        mShaders[(size_t)EInputType::Float]->LoadFromBinary(mContext, SHADER_F, sizeof(SHADER_F));
        mShaders[(size_t)EInputType::Int]->LoadFromBinary(mContext, SHADER_I, sizeof(SHADER_I));
        mShaders[(size_t)EInputType::Uint]->LoadFromBinary(mContext, SHADER_U, sizeof(SHADER_U));
    }
    void ComparerStage::CreatePipetteBuffer()
    {
        VkBufferUsageFlags              usage = VkBufferUsageFlagBits::VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        core::ManagedBuffer::CreateInfo ci(usage, sizeof(PipetteValue), VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
                                           VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, "Comparer.Pipette.Device");
        mPipetteBuffer.New(mContext, ci);
        mPipetteBuffer->Map(mPipetteMap);
    }
    void ComparerStage::CreateSubStage(SubStage& substage)
    {
        {  // Sampler
            VkSamplerCreateInfo samplerCi{
                .sType            = VkStructureType::VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
                .addressModeU     = VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_REPEAT,
                .addressModeV     = VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_REPEAT,
                .addressModeW     = VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_REPEAT,
                .anisotropyEnable = VK_FALSE,
                .compareEnable    = VK_FALSE,
                .minLod           = 0,
                .maxLod           = 0,
            };

            switch(substage.Input.Type)
            {
                case EInputType::Float:
                    samplerCi.magFilter = VkFilter::VK_FILTER_LINEAR;
                    samplerCi.minFilter = VkFilter::VK_FILTER_LINEAR;
                default:
                    samplerCi.magFilter = VkFilter::VK_FILTER_NEAREST;
                    samplerCi.minFilter = VkFilter::VK_FILTER_NEAREST;
            }

            substage.InputSampled.Init(mContext, substage.Input.Image, samplerCi);
        }
        {  // Descriptor Set
            substage.DescriptorSet.SetDescriptorAt(0, substage.InputSampled.GetVkDescriptorInfo(), VkDescriptorType::VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                                   VkShaderStageFlagBits::VK_SHADER_STAGE_COMPUTE_BIT);
            substage.DescriptorSet.SetDescriptorAt(1, mOutput.Get(), VkImageLayout::VK_IMAGE_LAYOUT_GENERAL, nullptr, VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                                                   VkShaderStageFlagBits::VK_SHADER_STAGE_COMPUTE_BIT);
            substage.DescriptorSet.SetDescriptorAt(2, mPipetteBuffer.Get(), VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VkShaderStageFlagBits::VK_SHADER_STAGE_COMPUTE_BIT);
            if(substage.DescriptorSet.Exists())
            {
                substage.DescriptorSet.Update();
            }
            else
            {
                substage.DescriptorSet.Create(mContext, "Comparer Stage Descriptor Set");
            }
        }
        {  // Pipeline Layout
            substage.PipelineLayout.AddDescriptorSetLayout(substage.DescriptorSet.GetDescriptorSetLayout());
            substage.PipelineLayout.AddPushConstantRange<PushConstant>(VkShaderStageFlagBits::VK_SHADER_STAGE_COMPUTE_BIT);
            substage.PipelineLayout.Build(mContext);
        }
        {  // Pipeline
            VkPipelineShaderStageCreateInfo shaderStageCi{.sType  = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                                                          .stage  = VkShaderStageFlagBits::VK_SHADER_STAGE_COMPUTE_BIT,
                                                          .module = *substage.Shader,
                                                          .pName  = "main"};

            VkComputePipelineCreateInfo pipelineCi{
                .sType  = VkStructureType::VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
                .stage  = shaderStageCi,
                .layout = substage.PipelineLayout,
            };

            AssertVkResult(mContext->DispatchTable().createComputePipelines(nullptr, 1U, &pipelineCi, nullptr, &substage.Pipeline));
        }
    }

    void ComparerStage::DestroySubStage(SubStage& substage, bool final)
    {
        if(!!mContext && !!mContext->Device && !!substage.Pipeline)
        {
            mContext->DispatchTable().destroyPipeline(substage.Pipeline, nullptr);
            substage.Pipeline = nullptr;
        }
        substage.Shader = nullptr;
        // substage.PipelineLayout.Destroy();
        if(final)
        {
            // substage.DescriptorSet.Destroy();
        }
        substage.InputSampled.Destroy();
        substage.Input = InputInfo{};
    }

    ComparerStage::~ComparerStage()
    {
        for(SubStage& substage : mSubStages)
        {
            DestroySubStage(substage, true);
        }
    }

#pragma endregion
#pragma region Runtime Update
    void ComparerStage::SetInput(uint32_t index, const InputInfo& input)
    {
        DestroySubStage(mSubStages[index], false);
        if(input.Image == nullptr)
        {
            mMixValue = index == 0 ? 0.f : 1.f;
            return;
        }

        mSubStages[index].Input  = input;
        mSubStages[index].Index  = index;
        mSubStages[index].Shader = mShaders[(size_t)input.Type].Get();

        if(!!mContext)
        {
            CreateSubStage(mSubStages[index]);
        }
    }

    void ComparerStage::HandleMouseMovedEvent(const osi::EventInputMouseMoved* event)
    {
        mMousePos = glm::ivec2(event->CurrentX, event->CurrentY);
        if(mFlipY)
        {
            mMousePos.y = mContext->WindowSwapchain->GetExtent().height - mMousePos.y - 1;
        }
    }

    void ComparerStage::OnResized(VkExtent2D extent)
    {
        RenderStage::OnResized(extent);
        for(SubStage& substage : mSubStages)
        {
            if(substage.DescriptorSet.Exists())
            {
                substage.DescriptorSet.SetDescriptorAt(0, substage.InputSampled.GetVkDescriptorInfo(), VkDescriptorType::VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                                       VkShaderStageFlagBits::VK_SHADER_STAGE_COMPUTE_BIT);
                substage.DescriptorSet.SetDescriptorAt(1, mOutput.Get(), VkImageLayout::VK_IMAGE_LAYOUT_GENERAL, nullptr, VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                                                       VkShaderStageFlagBits::VK_SHADER_STAGE_COMPUTE_BIT);
                substage.DescriptorSet.Update();
            }
        }
    }

#pragma endregion
#pragma region Render

    void ComparerStage::RecordFrame(VkCommandBuffer cmdBuffer, base::FrameRenderInfo& renderInfo)
    {
        {  // Get Pipette value (which might be very much out of date, but since it's for UI purposes only this is fine)
            memcpy(&mPipetteValue, mPipetteMap, sizeof(mPipetteValue));
        }
        if(!!mSubStages[0].Input.Image)
        {
            DispatchSubStage(mSubStages[0], cmdBuffer, renderInfo);
        }
        if(!!mSubStages[1].Input.Image)
        {
            DispatchSubStage(mSubStages[1], cmdBuffer, renderInfo);
        }
    }

    void ComparerStage::DispatchSubStage(SubStage& substage, VkCommandBuffer cmdBuffer, base::FrameRenderInfo& renderInfo)
    {
        {  // Barriers
            std::array<VkImageMemoryBarrier2, 2> vkBarriers;

            uint32_t index = 0;

            {
                core::ImageLayoutCache::Barrier2 barrier{.SrcStageMask     = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
                                                         .SrcAccessMask    = VK_ACCESS_2_MEMORY_WRITE_BIT,
                                                         .DstStageMask     = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                                                         .DstAccessMask    = VK_ACCESS_2_SHADER_READ_BIT,
                                                         .NewLayout        = VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                                         .SubresourceRange = VkImageSubresourceRange{.aspectMask = substage.Input.Aspect, .levelCount = 1, .layerCount = 1}};
                vkBarriers[index++] = (renderInfo.GetImageLayoutCache().MakeBarrier(substage.Input.Image, barrier));
            }
            {
                core::ImageLayoutCache::Barrier2 barrier{
                    .SrcStageMask  = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
                    .SrcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT,
                    .DstStageMask  = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                    .DstAccessMask = VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
                    .NewLayout     = VkImageLayout::VK_IMAGE_LAYOUT_GENERAL,
                };
                vkBarriers[index++] = (renderInfo.GetImageLayoutCache().MakeBarrier(mOutput.Get(), barrier));
            }

            VkDependencyInfo depInfo{
                .sType = VkStructureType::VK_STRUCTURE_TYPE_DEPENDENCY_INFO, .imageMemoryBarrierCount = (uint32_t)vkBarriers.size(), .pImageMemoryBarriers = vkBarriers.data()};

            vkCmdPipelineBarrier2(cmdBuffer, &depInfo);
        }
        {  // Bind
            mContext->DispatchTable().cmdBindPipeline(cmdBuffer, VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_COMPUTE, substage.Pipeline);

            VkDescriptorSet descriptorSet = substage.DescriptorSet.GetDescriptorSet();

            mContext->DispatchTable().cmdBindDescriptorSets(cmdBuffer, VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_COMPUTE, substage.PipelineLayout, 0U, 1U, &descriptorSet, 0U,
                                                              nullptr);
        }
        glm::uvec2 groupSize;
        uint32_t   writeOffset;
        VkExtent2D screenSize = mDomain->GetExtent();
        {  // Group size & Write Offset
            const glm::uvec2 localSize = glm::uvec2(16, 16);

            if(substage.Index == 0)
            {
                writeOffset = 0;
                groupSize.x = ((uint32_t)(screenSize.width * mMixValue) + localSize.x - 1) / localSize.x;
            }
            else
            {
                writeOffset = ((uint32_t)(screenSize.width * mMixValue) / localSize.x) * localSize.x;
                groupSize.x = (screenSize.width - writeOffset + localSize.x - 1) / localSize.x;
            }
            groupSize.y = (screenSize.height + localSize.y - 1) / localSize.y;
        }
        {  // PushConstant
            PushConstant pushC{.Scale       = substage.Input.Scale,
                               .MousePos    = mMousePos,
                               .Channels    = substage.Input.ChannelCount,
                               .Mix         = mMixValue,
                               .WriteOffset = writeOffset,
                               .WriteLeft   = (substage.Index == 0) ? VK_TRUE : VK_FALSE};

            mContext->DispatchTable().cmdPushConstants(cmdBuffer, substage.PipelineLayout, VkShaderStageFlagBits::VK_SHADER_STAGE_COMPUTE_BIT, 0U, sizeof(PushConstant), &pushC);
        }
        {  // Dispatch
            mContext->DispatchTable().cmdDispatch(cmdBuffer, groupSize.x, groupSize.y, 1U);
        }
    }

#pragma endregion
}  // namespace foray::stages
