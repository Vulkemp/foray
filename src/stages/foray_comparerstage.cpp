#include "foray_comparerstage.hpp"
#include "../osi/foray_event.hpp"
#include "../osi/foray_inputdevice.hpp"
#include "../osi/foray_osmanager.hpp"

namespace foray::stages {
#pragma region Init
    void ComparerStage::Init(core::Context* context)
    {
        mContext = context;
        SetInput(0, &mMissingInput, 4);
        SetInput(1, &mMissingInput, 4);
        ComputeStage::Init(context);
    }

    void ComparerStage::ApiInitShader()
    {
        mShader.LoadFromSource(mContext, "shaders/comparerstage.comp");
    }
    void ComparerStage::ApiCreateDescriptorSetLayout()
    {
        UpdateDescriptorSet();
        mDescriptorSet.Create(mContext, "Comparer Stage Descriptor Set");
    }
    void ComparerStage::ApiCreatePipelineLayout()
    {
        mPipelineLayout.AddDescriptorSetLayout(mDescriptorSet.GetDescriptorSetLayout());
        mPipelineLayout.AddPushConstantRange<PushConstant>(VkShaderStageFlagBits::VK_SHADER_STAGE_COMPUTE_BIT);
        mPipelineLayout.Build(mContext);
    }
    void ComparerStage::CreateResolutionDependentComponents()
    {
        VkImageUsageFlags usage =
            VkImageUsageFlagBits::VK_IMAGE_USAGE_STORAGE_BIT | VkImageUsageFlagBits::VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VkImageUsageFlagBits::VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        VkExtent3D                     extent{mContext->GetSwapchainSize().width, mContext->GetSwapchainSize().height, 1};
        core::ManagedImage::CreateInfo ci(OutputName, usage, VkFormat::VK_FORMAT_R32G32B32A32_SFLOAT, extent);
        mOutput.Create(mContext, ci);
        mImageOutputs[mOutput.GetName()] = &mOutput;
    }
    void ComparerStage::CreateFixedSizeComponents()
    {
        {
            VkBufferUsageFlags usage = VkBufferUsageFlagBits::VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            core::ManagedBuffer::ManagedBufferCreateInfo ci(usage, sizeof(glm::vec4), VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
                                                            VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, "Comparer.Pipette.Device");
            mPipetteBuffer.Create(mContext, ci);
            mPipetteBuffer.Map(mPipetteMap);
        }
        {
            VkImageUsageFlags usage = VkImageUsageFlagBits::VK_IMAGE_USAGE_SAMPLED_BIT | VkImageUsageFlagBits::VK_IMAGE_USAGE_TRANSFER_DST_BIT;
            core::ManagedImage::CreateInfo ci("Missing Image", usage, VkFormat::VK_FORMAT_R32G32B32A32_SFLOAT, VkExtent3D{1, 1, 1});
            mMissingInput.Create(mContext, ci);
            glm::vec4 color(0.5, 0.f, 0.5f, 1.f);
            mMissingInput.WriteDeviceLocalData(&color, sizeof(color), VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        }
        ComputeStage::CreateFixedSizeComponents();
    }
    void ComparerStage::DestroyFixedComponents()
    {
        if(mPipetteBuffer.Exists())
        {
            mPipetteBuffer.Unmap();
        }
        mPipetteBuffer.Destroy();
        mMissingInput.Destroy();
        ComputeStage::DestroyFixedComponents();
    }

    void ComparerStage::DestroyResolutionDependentComponents()
    {
        mOutput.Destroy();
    }

#pragma endregion
#pragma region Runtime Update
    void ComparerStage::SetInput(uint32_t index, core::ManagedImage* image, uint32_t channelCount, glm::vec4 scale)
    {
        if (image == nullptr)
        {
            image = &mMissingInput;
        }
        mInputs[index] = image;
        mInputsSampled[index].Destroy();

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

        switch(image->GetFormat())
        {
            case VkFormat::VK_FORMAT_R32G32_SFLOAT:
            case VkFormat::VK_FORMAT_R32G32B32A32_SFLOAT:
                samplerCi.magFilter = VkFilter::VK_FILTER_LINEAR;
                samplerCi.minFilter = VkFilter::VK_FILTER_LINEAR;
            default:
                samplerCi.magFilter = VkFilter::VK_FILTER_NEAREST;
                samplerCi.minFilter = VkFilter::VK_FILTER_NEAREST;
        }

        mPushC.Channels[index] = channelCount;
        mPushC.Scale[index]    = scale;

        if(!!mContext)
        {
            mInputsSampled[index].Init(mContext, mInputs[index], samplerCi);
        }

        if(mDescriptorSet.Exists())
        {
            UpdateDescriptorSet();
        }
    }

    void ComparerStage::UpdateDescriptorSet()
    {
        std::vector<VkDescriptorImageInfo> inputInfos;
        inputInfos.reserve(2);

        mPushC.InputCount = 0;

        for(core::CombinedImageSampler& sampler : mInputsSampled)
        {
            VkDescriptorImageInfo info = sampler.GetVkDescriptorInfo();
            if(!!info.imageView && !!info.sampler)
            {
                mPushC.InputCount++;
                inputInfos.push_back(info);
            }
        }

        mDescriptorSet.SetDescriptorAt(0, inputInfos, VkDescriptorType::VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VkShaderStageFlagBits::VK_SHADER_STAGE_COMPUTE_BIT);
        mDescriptorSet.SetDescriptorAt(1, mOutput, VkImageLayout::VK_IMAGE_LAYOUT_GENERAL, nullptr, VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                                       VkShaderStageFlagBits::VK_SHADER_STAGE_COMPUTE_BIT);
        mDescriptorSet.SetDescriptorAt(2, mPipetteBuffer, VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VkShaderStageFlagBits::VK_SHADER_STAGE_COMPUTE_BIT);

        if(mDescriptorSet.Exists())
        {
            mDescriptorSet.Update();
        }
    }

    void ComparerStage::HandleEvent(const osi::Event* event)
    {
        const osi::EventInputMouseMoved* mouseMove = dynamic_cast<const osi::EventInputMouseMoved*>(event);
        if(!!mouseMove)
        {
            mPushC.MousePos = glm::ivec2(mouseMove->CurrentX, mouseMove->CurrentY);
        }
    }

    void ComparerStage::OnResized(const VkExtent2D& extent)
    {
        mOutput.Resize(VkExtent3D{extent.width, extent.height, 1});
        UpdateDescriptorSet();
    }

#pragma endregion
#pragma region Render

    void ComparerStage::RecordFrame(VkCommandBuffer cmdBuffer, base::FrameRenderInfo& renderInfo)
    {
        {  // Get Pipette value (which might be up to 1 frame out of date, but since it's for UI purposes only this is fine)
            memcpy(&mPipetteValue, mPipetteMap, sizeof(mPipetteValue));
        }
        {  // Barriers
            std::array<VkImageMemoryBarrier2, 3> vkBarriers;

            uint32_t index = 0;

            for(core::ManagedImage* image : mInputs)
            {
                core::ImageLayoutCache::Barrier2 barrier{
                    .SrcStageMask  = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
                    .SrcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT,
                    .DstStageMask  = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                    .DstAccessMask = VK_ACCESS_2_SHADER_READ_BIT,
                    .NewLayout     = VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                };
                vkBarriers[index++] = (renderInfo.GetImageLayoutCache().Set(image, barrier));
            }

            {
                core::ImageLayoutCache::Barrier2 barrier{
                    .SrcStageMask  = VK_PIPELINE_STAGE_2_NONE,
                    .SrcAccessMask = VK_ACCESS_2_NONE,
                    .DstStageMask  = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                    .DstAccessMask = VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
                    .NewLayout     = VkImageLayout::VK_IMAGE_LAYOUT_GENERAL,
                };
                vkBarriers[index++] = (renderInfo.GetImageLayoutCache().Set(mOutput, barrier));
            }

            VkDependencyInfo depInfo{
                .sType = VkStructureType::VK_STRUCTURE_TYPE_DEPENDENCY_INFO, .imageMemoryBarrierCount = (uint32_t)vkBarriers.size(), .pImageMemoryBarriers = vkBarriers.data()};

            vkCmdPipelineBarrier2(cmdBuffer, &depInfo);
        }
        {  // Bind
            mContext->VkbDispatchTable->cmdBindPipeline(cmdBuffer, VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_COMPUTE, mPipeline);

            VkDescriptorSet descriptorSet = mDescriptorSet.GetDescriptorSet();

            mContext->VkbDispatchTable->cmdBindDescriptorSets(cmdBuffer, VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_COMPUTE, mPipelineLayout, 0U, 1U, &descriptorSet, 0U, nullptr);
        }
        {  // PushConstant
            mContext->VkbDispatchTable->cmdPushConstants(cmdBuffer, mPipelineLayout, VkShaderStageFlagBits::VK_SHADER_STAGE_COMPUTE_BIT, 0U, sizeof(PushConstant), &mPushC);
        }
        {  // Dispatch
            const glm::uvec2 localSize = glm::uvec2(16, 16);

            VkExtent2D screenSize = mContext->GetSwapchainSize();
            glm::uvec3 groupSize((screenSize.width + localSize.x - 1) / localSize.x, (screenSize.height + localSize.y - 1) / localSize.y, 1);

            mContext->VkbDispatchTable->cmdDispatch(cmdBuffer, groupSize.x, groupSize.y, groupSize.z);
        }
    }

#pragma endregion
}  // namespace foray::stages
