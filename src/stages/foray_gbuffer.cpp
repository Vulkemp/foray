#include "foray_gbuffer.hpp"
#include "../bench/foray_devicebenchmark.hpp"
#include "../core/foray_shadermanager.hpp"
#include "../scene/components/foray_meshinstance.hpp"
#include "../scene/globalcomponents/foray_cameramanager.hpp"
#include "../scene/globalcomponents/foray_drawmanager.hpp"
#include "../scene/globalcomponents/foray_geometrymanager.hpp"
#include "../scene/globalcomponents/foray_materialmanager.hpp"
#include "../scene/globalcomponents/foray_texturemanager.hpp"
#include "../util/foray_pipelinebuilder.hpp"
#include "../util/foray_shaderstagecreateinfos.hpp"

const uint32_t GBUFFER_SHADER_VERT[] =
#include "foray_gbuffer.vert.spv.h"
    ;
const uint32_t GBUFFER_SHADER_FRAG[] =
#include "foray_gbuffer.frag.spv.h"
    ;

namespace foray::stages {
    inline constexpr std::string_view OutputNames[] = {GBufferStage::PositionOutputName, GBufferStage::NormalOutputName,      GBufferStage::AlbedoOutputName,
                                                       GBufferStage::MotionOutputName,   GBufferStage::MaterialIdxOutputName, GBufferStage::MeshInstanceIdOutputName,
                                                       GBufferStage::LinearZOutputName,  GBufferStage::DepthOutputName};

#pragma region Init

    // Heavily inspired from Sascha Willems' "deferred" vulkan example
    void GBufferStage::Init(core::Context* context, scene::Scene* scene, std::string_view vertexShaderPath, std::string_view fragmentShaderPath, bench::DeviceBenchmark* benchmark)
    {
        mContext            = context;
        mScene              = scene;
        mVertexShaderPath   = vertexShaderPath;
        mFragmentShaderPath = fragmentShaderPath;

        CreateImages();
        PrepareRenderpass();
        if(!!benchmark)
        {
            mBenchmark = benchmark;
            std::vector<const char*> timestampNames(
                {bench::BenchmarkTimestamp::BEGIN, TIMESTAMP_VERT_BEGIN, TIMESTAMP_VERT_END, TIMESTAMP_FRAG_BEGIN, TIMESTAMP_FRAG_END, bench::BenchmarkTimestamp::END});
            mBenchmark->Create(mContext, timestampNames);
        }
        SetupDescriptors();
        CreateDescriptorSets();
        CreatePipelineLayout();
        CreatePipeline();
    }

    void GBufferStage::CreateImages()
    {
        static const VkFormat colorFormat    = VK_FORMAT_R16G16B16A16_SFLOAT;
        static const VkFormat geometryFormat = VK_FORMAT_R16G16B16A16_SFLOAT;

        static const VkImageUsageFlags imageUsageFlags =
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

        VkExtent2D extent = mContext->GetSwapchainSize();

        VkClearValue defaultClearValue = {VkClearColorValue{{0, 0, 0, 0}}};

        for(int32_t i = 0; i < (int32_t)EOutput::MaxEnum; i++)
        {
            PerImageInfo& info                         = mImageInfos[i];
            info.Output                                = (EOutput)i;
            info.ClearValue                            = defaultClearValue;
            mImageOutputs[std::string(OutputNames[i])] = &(info.Image);
        }

        {  // Position
            mImageInfos[(size_t)EOutput::Position].Image.Create(mContext, imageUsageFlags, geometryFormat, extent, OutputNames[(size_t)EOutput::Position]);
        }

        {  // Normal
            mImageInfos[(size_t)EOutput::Normal].Image.Create(mContext, imageUsageFlags, geometryFormat, extent, OutputNames[(size_t)EOutput::Normal]);
        }

        {  // Albedo
            mImageInfos[(size_t)EOutput::Albedo].Image.Create(mContext, imageUsageFlags, colorFormat, extent, OutputNames[(size_t)EOutput::Albedo]);
        }

        {  // Motion
            mImageInfos[(size_t)EOutput::Motion].Image.Create(mContext, imageUsageFlags, VK_FORMAT_R16G16_SFLOAT, extent, OutputNames[(size_t)EOutput::Motion]);
        }

        {  // Material
            mImageInfos[(size_t)EOutput::MaterialIdx].Image.Create(mContext, imageUsageFlags, VK_FORMAT_R32_SINT, extent, OutputNames[(size_t)EOutput::MaterialIdx]);
            mImageInfos[(size_t)EOutput::MaterialIdx].ClearValue.color = VkClearColorValue{.int32={-1, 0, 0, 0}};
        }

        {  // MeshId
            mImageInfos[(size_t)EOutput::MeshInstanceIdx].Image.Create(mContext, imageUsageFlags, VK_FORMAT_R32_UINT, extent, OutputNames[(size_t)EOutput::MeshInstanceIdx]);
            mImageInfos[(size_t)EOutput::MeshInstanceIdx].ClearValue.color = VkClearColorValue{.uint32{std::numeric_limits<uint32_t>().max(), 0, 0, 0}};
        }

        {  // LinearZ
            mImageInfos[(size_t)EOutput::LinearZ].Image.Create(mContext, imageUsageFlags, VK_FORMAT_R16G16_SFLOAT, extent, OutputNames[(size_t)EOutput::LinearZ]);
            mImageInfos[(size_t)EOutput::LinearZ].ClearValue.color = VkClearColorValue{.float32{std::numeric_limits<fp32_t>().max(), 0.f, 0.f, 0.f}};
        }

        {  // Depth
            VkImageUsageFlags depthUsage =
                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
            core::ManagedImage::CreateInfo ci(depthUsage, VK_FORMAT_D32_SFLOAT, extent, OutputNames[(size_t)EOutput::Depth]);
            ci.ImageViewCI.subresourceRange.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_DEPTH_BIT;
            mImageInfos[(size_t)EOutput::Depth].Image.Create(mContext, ci);
            mImageInfos[(size_t)EOutput::Depth].ClearValue.depthStencil = VkClearDepthStencilValue{1.f, 0};
        }

        {  // Pre-transfer to correct layout
            core::HostSyncCommandBuffer commandBuffer;
            commandBuffer.Create(mContext);
            commandBuffer.Begin();

            std::array<VkImageMemoryBarrier2, (size_t)EOutput::MaxEnum> barriers;

            VkImageMemoryBarrier2 attachmentMemBarrier{
                .sType               = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                .srcStageMask        = VK_PIPELINE_STAGE_2_NONE,
                .srcAccessMask       = VK_ACCESS_2_NONE,
                .dstStageMask        = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                .dstAccessMask       = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                .oldLayout           = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED,
                .newLayout           = VkImageLayout::VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .subresourceRange =
                    VkImageSubresourceRange{
                        .aspectMask     = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT,
                        .baseMipLevel   = 0,
                        .levelCount     = VK_REMAINING_MIP_LEVELS,
                        .baseArrayLayer = 0,
                        .layerCount     = VK_REMAINING_ARRAY_LAYERS,
                    },
            };

            for(int32_t i = 0; i < (int32_t)EOutput::Depth; i++)
            {
                PerImageInfo&          info    = mImageInfos[i];
                VkImageMemoryBarrier2& barrier = barriers[i];

                barrier       = attachmentMemBarrier;
                barrier.image = info.Image.GetImage();
            }


            barriers[(size_t)EOutput::Depth] = VkImageMemoryBarrier2{
                .sType               = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                .srcStageMask        = VK_PIPELINE_STAGE_2_NONE,
                .srcAccessMask       = VK_ACCESS_2_NONE,
                .dstStageMask        = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,
                .dstAccessMask       = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                .oldLayout           = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED,
                .newLayout           = VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image               = mImageInfos[(size_t)EOutput::Depth].Image.GetImage(),
                .subresourceRange =
                    VkImageSubresourceRange{
                        .aspectMask     = VkImageAspectFlagBits::VK_IMAGE_ASPECT_DEPTH_BIT,
                        .baseMipLevel   = 0,
                        .levelCount     = VK_REMAINING_MIP_LEVELS,
                        .baseArrayLayer = 0,
                        .layerCount     = VK_REMAINING_ARRAY_LAYERS,
                    },
            };

            VkDependencyInfo depInfo{
                .sType = VkStructureType::VK_STRUCTURE_TYPE_DEPENDENCY_INFO, .imageMemoryBarrierCount = (uint32_t)barriers.size(), .pImageMemoryBarriers = barriers.data()};

            vkCmdPipelineBarrier2(commandBuffer, &depInfo);

            commandBuffer.SubmitAndWait();
        }
    }

    void GBufferStage::PrepareRenderpass()
    {
        std::array<VkAttachmentDescription, (size_t)EOutput::MaxEnum> attachmentDescriptions;

        std::array<VkAttachmentReference, (size_t)EOutput::Depth> colorAttachmentReferences;

        std::array<VkImageView, (size_t)EOutput::MaxEnum> attachmentViews;

        VkAttachmentReference depthAttachmentReference{};

        attachmentDescriptions.fill({});
        colorAttachmentReferences.fill({});
        attachmentViews.fill({});

        // Color Outputs

        for(int32_t i = 0; i < (int32_t)EOutput::Depth; i++)
        {
            core::ManagedImage& image                = mImageInfos[i].Image;
            attachmentDescriptions[i].samples        = image.GetSampleCount();
            attachmentDescriptions[i].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
            attachmentDescriptions[i].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
            attachmentDescriptions[i].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachmentDescriptions[i].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachmentDescriptions[i].initialLayout  = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            attachmentDescriptions[i].finalLayout    = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            attachmentDescriptions[i].format         = image.GetFormat();

            colorAttachmentReferences[i] = {(uint32_t)i, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
            attachmentViews[i]           = image.GetImageView();
        }


        // Depth Output
        core::ManagedImage& depthImage = mImageInfos[(size_t)EOutput::Depth].Image;

        VkAttachmentDescription depthAttachmentDescription{};
        depthAttachmentDescription.samples        = depthImage.GetSampleCount();
        depthAttachmentDescription.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachmentDescription.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
        depthAttachmentDescription.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
        depthAttachmentDescription.initialLayout  = VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        depthAttachmentDescription.finalLayout    = VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        depthAttachmentDescription.format         = depthImage.GetFormat();

        // the depth attachment gets the final id (one higher than the highest color attachment id)
        depthAttachmentReference                       = {(uint32_t)EOutput::Depth, VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
        attachmentDescriptions[(size_t)EOutput::Depth] = depthAttachmentDescription;
        attachmentViews[(size_t)EOutput::Depth]        = depthImage.GetImageView();

        // Subpass description
        VkSubpassDescription subpass    = {};
        subpass.pipelineBindPoint       = VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount    = colorAttachmentReferences.size();
        subpass.pColorAttachments       = colorAttachmentReferences.data();
        subpass.pDepthStencilAttachment = &depthAttachmentReference;

        VkSubpassDependency subPassDependencies[2] = {};
        subPassDependencies[0].srcSubpass          = VK_SUBPASS_EXTERNAL;
        subPassDependencies[0].dstSubpass          = 0;
        subPassDependencies[0].srcStageMask        = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        subPassDependencies[0].dstStageMask        = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        subPassDependencies[0].srcAccessMask       = VK_ACCESS_MEMORY_READ_BIT;
        subPassDependencies[0].dstAccessMask       = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        subPassDependencies[0].dependencyFlags     = VK_DEPENDENCY_BY_REGION_BIT;

        subPassDependencies[1].srcSubpass      = 0;
        subPassDependencies[1].dstSubpass      = VK_SUBPASS_EXTERNAL;
        subPassDependencies[1].srcStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        subPassDependencies[1].dstStageMask    = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        subPassDependencies[1].srcAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        subPassDependencies[1].dstAccessMask   = VK_ACCESS_MEMORY_READ_BIT;
        subPassDependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        VkRenderPassCreateInfo renderPassInfo = {};
        renderPassInfo.sType                  = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.pAttachments           = attachmentDescriptions.data();
        renderPassInfo.attachmentCount        = static_cast<uint32_t>(attachmentDescriptions.size());
        renderPassInfo.subpassCount           = 1;
        renderPassInfo.pSubpasses             = &subpass;
        renderPassInfo.dependencyCount        = 2;
        renderPassInfo.pDependencies          = subPassDependencies;
        AssertVkResult(vkCreateRenderPass(mContext->Device(), &renderPassInfo, nullptr, &mRenderpass));

        VkFramebufferCreateInfo fbufCreateInfo = {};
        fbufCreateInfo.sType                   = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fbufCreateInfo.pNext                   = NULL;
        fbufCreateInfo.renderPass              = mRenderpass;
        fbufCreateInfo.pAttachments            = attachmentViews.data();
        fbufCreateInfo.attachmentCount         = static_cast<uint32_t>(attachmentViews.size());
        fbufCreateInfo.width                   = mContext->GetSwapchainSize().width;
        fbufCreateInfo.height                  = mContext->GetSwapchainSize().height;
        fbufCreateInfo.layers                  = 1;
        AssertVkResult(vkCreateFramebuffer(mContext->Device(), &fbufCreateInfo, nullptr, &mFrameBuffer));
    }

    void GBufferStage::SetupDescriptors()
    {
        auto materialBuffer = mScene->GetComponent<scene::gcomp::MaterialManager>();
        auto textureStore   = mScene->GetComponent<scene::gcomp::TextureManager>();
        auto cameraManager  = mScene->GetComponent<scene::gcomp::CameraManager>();
        auto drawDirector   = mScene->GetComponent<scene::gcomp::DrawDirector>();
        mDescriptorSet.SetDescriptorAt(0, materialBuffer->GetVkDescriptorInfo(), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT);
        mDescriptorSet.SetDescriptorAt(1, textureStore->GetDescriptorInfos(), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
        mDescriptorSet.SetDescriptorAt(2, cameraManager->GetVkDescriptorInfo(), VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);
        mDescriptorSet.SetDescriptorAt(3, drawDirector->GetCurrentTransformsDescriptorInfo(), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);
        mDescriptorSet.SetDescriptorAt(4, drawDirector->GetPreviousTransformsDescriptorInfo(), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);
    }

    void GBufferStage::CreateDescriptorSets()
    {
        mDescriptorSet.Create(mContext, "GBuffer_DescriptorSet");
    }

    void GBufferStage::CreatePipelineLayout()
    {
        mPipelineLayout.AddDescriptorSetLayout(mDescriptorSet.GetDescriptorSetLayout());
        mPipelineLayout.AddPushConstantRange<scene::DrawPushConstant>(VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT | VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT);
        mPipelineLayout.Build(mContext);
    }

    void GBufferStage::CreatePipeline()
    {
        // shader stages
        if(mVertexShaderPath.size() > 0)
        {
            mShaderKeys.push_back(mContext->ShaderMan->CompileShader(mVertexShaderPath, mVertexShaderModule));
        }
        else
        {
            mVertexShaderModule.LoadFromBinary(mContext, GBUFFER_SHADER_VERT, sizeof(GBUFFER_SHADER_VERT));
        }
        if(mFragmentShaderPath.size() > 0)
        {
            mShaderKeys.push_back(mContext->ShaderMan->CompileShader(mFragmentShaderPath, mFragmentShaderModule));
        }
        {
            mFragmentShaderModule.LoadFromBinary(mContext, GBUFFER_SHADER_FRAG, sizeof(GBUFFER_SHADER_FRAG));
        }
        util::ShaderStageCreateInfos shaderStageCreateInfos;
        shaderStageCreateInfos.Add(VK_SHADER_STAGE_VERTEX_BIT, mVertexShaderModule).Add(VK_SHADER_STAGE_FRAGMENT_BIT, mFragmentShaderModule);

        // vertex layout
        scene::VertexInputStateBuilder vertexInputStateBuilder;
        vertexInputStateBuilder.AddVertexComponentBinding(scene::EVertexComponent::Position);
        vertexInputStateBuilder.AddVertexComponentBinding(scene::EVertexComponent::Normal);
        vertexInputStateBuilder.AddVertexComponentBinding(scene::EVertexComponent::Tangent);
        vertexInputStateBuilder.AddVertexComponentBinding(scene::EVertexComponent::Uv);
        vertexInputStateBuilder.Build();

        // clang-format off
        mPipeline = util::PipelineBuilder()
            .SetContext(mContext)
            // Blend attachment states required for all color attachments
            // This is important, as color write mask will otherwise be 0x0 and you
            // won't see anything rendered to the attachment
            .SetColorAttachmentBlendCount((size_t)EOutput::MaxEnum - 1)
            .SetPipelineLayout(mPipelineLayout.GetPipelineLayout())
            .SetVertexInputStateBuilder(&vertexInputStateBuilder)
            .SetShaderStageCreateInfos(shaderStageCreateInfos.Get())
            .SetPipelineCache(mContext->PipelineCache)
            .SetRenderPass(mRenderpass)
            .Build();
        // clang-format on
    }

#pragma endregion
#pragma region Destroy

    void GBufferStage::Destroy()
    {
        if(!!mBenchmark)
        {
            mBenchmark->Destroy();
        }
        VkDevice device = mContext->Device();
        if(mPipeline)
        {
            vkDestroyPipeline(device, mPipeline, nullptr);
            mPipeline = nullptr;
        }
        mPipelineLayout.Destroy();
        mDescriptorSet.Destroy();
        mVertexShaderModule.Destroy();
        mFragmentShaderModule.Destroy();
        RenderStage::DestroyOutputImages();
        DestroyFrameBufferAndRenderpass();
    }

    void GBufferStage::DestroyFrameBufferAndRenderpass()
    {
        if(mFrameBuffer)
        {
            vkDestroyFramebuffer(mContext->Device(), mFrameBuffer, nullptr);
            mFrameBuffer = nullptr;
        }
        if(mRenderpass)
        {
            vkDestroyRenderPass(mContext->Device(), mRenderpass, nullptr);
            mRenderpass = nullptr;
        }
    }

#pragma endregion
#pragma region Runtime Update

    void GBufferStage::Resize(const VkExtent2D& extent)
    {
        DestroyFrameBufferAndRenderpass();
        VkExtent3D imageExtent{.width = extent.width, .height = extent.height, .depth = 1};
        for(PerImageInfo& info : mImageInfos)
        {
            info.Image.Resize(imageExtent);
        }
        PrepareRenderpass();
    }

#pragma endregion
#pragma region Misc

    core::ManagedImage* GBufferStage::GetImageEOutput(EOutput output, bool noThrow)
    {
        return RenderStage::GetImageOutput(OutputNames[(size_t)output], noThrow);
    }

#pragma endregion

    void GBufferStage::RecordFrame(VkCommandBuffer cmdBuffer, base::FrameRenderInfo& renderInfo)
    {

        uint32_t frameNum = renderInfo.GetFrameNumber();
        if(!!mBenchmark)
        {
            mBenchmark->CmdResetQuery(cmdBuffer, frameNum);
            mBenchmark->CmdWriteTimestamp(cmdBuffer, frameNum, bench::BenchmarkTimestamp::BEGIN, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
        }

        VkImageMemoryBarrier2 attachmentMemBarrier{
            .sType               = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .srcStageMask        = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
            .srcAccessMask       = VK_ACCESS_2_NONE,
            .dstStageMask        = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
            .dstAccessMask       = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
            .oldLayout           = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED,  // We do not care about the contents of all attachments as they're cleared and rewritten completely
            .newLayout           = VkImageLayout::VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .subresourceRange =
                VkImageSubresourceRange{
                    .aspectMask     = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel   = 0,
                    .levelCount     = 1,
                    .baseArrayLayer = 0,
                    .layerCount     = 1,
                },
        };

        std::array<VkImageMemoryBarrier2, (size_t)EOutput::MaxEnum> imgBarriers;

        for(int32_t i = 0; i < (int32_t)EOutput::Depth; i++)
        {
            PerImageInfo&          info    = mImageInfos[i];
            VkImageMemoryBarrier2& barrier = imgBarriers[i];

            barrier       = attachmentMemBarrier;
            barrier.image = info.Image.GetImage();
        }
        VkImageMemoryBarrier2& depthBarrier      = imgBarriers[(size_t)EOutput::Depth];
        depthBarrier                             = attachmentMemBarrier;
        depthBarrier.dstStageMask                = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
        depthBarrier.dstAccessMask               = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT_KHR | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT_KHR;
        depthBarrier.newLayout                   = VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        depthBarrier.subresourceRange.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_DEPTH_BIT;
        depthBarrier.image                       = mImageInfos[(size_t)EOutput::Depth].Image.GetImage();

        std::vector<VkBufferMemoryBarrier2> bufferBarriers;

        VkBufferMemoryBarrier2 bufferBarrier{.sType               = VkStructureType::VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
                                             .srcStageMask        = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
                                             .srcAccessMask       = VK_ACCESS_2_MEMORY_WRITE_BIT,
                                             .dstStageMask        = VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT,
                                             .dstAccessMask       = VK_ACCESS_2_SHADER_READ_BIT,
                                             .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                                             .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                                             .offset              = 0,
                                             .size                = VK_WHOLE_SIZE};

        auto materialBuffer = mScene->GetComponent<scene::gcomp::MaterialManager>();
        auto cameraManager  = mScene->GetComponent<scene::gcomp::CameraManager>();
        auto drawDirector   = mScene->GetComponent<scene::gcomp::DrawDirector>();

        bufferBarrier.buffer = materialBuffer->GetVkBuffer();
        bufferBarriers.push_back(bufferBarrier);
        bufferBarriers.push_back(cameraManager->GetUbo().MakeBarrierPrepareForRead(VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT));
        bufferBarrier.buffer = drawDirector->GetCurrentTransformsVkBuffer();
        bufferBarriers.push_back(bufferBarrier);
        bufferBarrier.buffer = drawDirector->GetPreviousTransformsVkBuffer();
        bufferBarriers.push_back(bufferBarrier);

        VkDependencyInfo depInfo{
            .sType                    = VkStructureType::VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .dependencyFlags          = VkDependencyFlagBits::VK_DEPENDENCY_BY_REGION_BIT,
            .bufferMemoryBarrierCount = (uint32_t)bufferBarriers.size(),
            .pBufferMemoryBarriers    = bufferBarriers.data(),
            .imageMemoryBarrierCount  = (uint32_t)imgBarriers.size(),
            .pImageMemoryBarriers     = imgBarriers.data(),
        };

        vkCmdPipelineBarrier2(cmdBuffer, &depInfo);

        std::array<VkClearValue, (size_t)EOutput::MaxEnum> clearValues;

        for(int32_t i = 0; i < (int32_t)EOutput::MaxEnum; i++)
        {
            clearValues[i] = mImageInfos[i].ClearValue;
        }

        VkRenderPassBeginInfo renderPassBeginInfo{};
        renderPassBeginInfo.sType             = VkStructureType::VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassBeginInfo.renderPass        = mRenderpass;
        renderPassBeginInfo.framebuffer       = mFrameBuffer;
        renderPassBeginInfo.renderArea.extent = mContext->GetSwapchainSize();
        renderPassBeginInfo.clearValueCount   = static_cast<uint32_t>(clearValues.size());
        renderPassBeginInfo.pClearValues      = clearValues.data();

        vkCmdBeginRenderPass(cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

        VkViewport viewport{0.f, 0.f, (float)mContext->GetSwapchainSize().width, (float)mContext->GetSwapchainSize().height, 0.0f, 1.0f};
        vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);

        VkRect2D scissor{VkOffset2D{}, VkExtent2D{mContext->GetSwapchainSize()}};
        vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);

        vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipeline);

        VkDescriptorSet descriptorSet = mDescriptorSet.GetDescriptorSet();
        // Instanced object
        vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipelineLayout, 0, 1, &descriptorSet, 0, nullptr);

        if(!!mBenchmark)
        {
            mBenchmark->CmdWriteTimestamp(cmdBuffer, frameNum, TIMESTAMP_VERT_BEGIN, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_VERTEX_SHADER_BIT);
            mBenchmark->CmdWriteTimestamp(cmdBuffer, frameNum, TIMESTAMP_VERT_END, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT);
            mBenchmark->CmdWriteTimestamp(cmdBuffer, frameNum, TIMESTAMP_FRAG_BEGIN, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
            mBenchmark->CmdWriteTimestamp(cmdBuffer, frameNum, TIMESTAMP_FRAG_END, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT);
        }

        mScene->Draw(renderInfo, mPipelineLayout, cmdBuffer);

        vkCmdEndRenderPass(cmdBuffer);
        if(!!mBenchmark)
        {
            mBenchmark->CmdWriteTimestamp(cmdBuffer, frameNum, bench::BenchmarkTimestamp::END, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
        }

        // The GBuffer determines the images layouts

        for(int32_t i = 0; i < (int32_t)EOutput::Depth; i++)
        {
            renderInfo.GetImageLayoutCache().Set(mImageInfos[i].Image, VkImageLayout::VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        }
        renderInfo.GetImageLayoutCache().Set(mImageInfos[(size_t)EOutput::Depth].Image, VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    }
}  // namespace foray::stages