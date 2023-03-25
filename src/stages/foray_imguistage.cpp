#include "foray_imguistage.hpp"
#include "../core/foray_commandbuffer.hpp"
#include "../core/foray_shadermodule.hpp"
#include "../osi/foray_osmanager.hpp"
#include "../osi/foray_window.hpp"
#include "../util/foray_pipelinebuilder.hpp"

#include <imgui/imgui.h>
#include <imgui/imgui_impl_sdl.h>
#include <imgui/imgui_impl_vulkan.h>


namespace foray::stages {
    void ImguiStage::InitForImage(core::Context* context, RenderDomain* domain, core::ManagedImage* backgroundImage, int32_t resizeOrder)
    {
        Destroy();
        mContext     = context;
        mTargetImage = backgroundImage;
        mOnSdlEvent.Set(context->OsManager->OnEventRawSDL(), [this](const osi::EventRawSDL* event) { this->HandleSdlEvent(event); });

        Assert(!!mTargetImage, "Imgui stage can only init when the background image is set!");

        PrepareRenderpass();
        InitImgui();
    }

    void ImguiStage::InitForSwapchain(core::Context* context, RenderDomain* domain, int32_t resizeOrder)
    {
        Destroy();
        RenderStage::InitCallbacks(context, domain, resizeOrder);
        mOnSdlEvent.Set(context->OsManager->OnEventRawSDL(), [this](const osi::EventRawSDL* event) { this->HandleSdlEvent(event); });
        mTargetImage = nullptr;

        PrepareRenderpass();
        InitImgui();
    }

    void ImguiStage::InitImgui()
    {

        // Clear values for all attachments written in the fragment shader
        mClearValue.color = {{0.0f, 0.0f, 0.0f, 1.0f}};


        // Yes, this is actually how they do it https://github.com/ocornut/imgui/blob/master/examples/example_sdl_vulkan/main.cpp#L175-L187
        VkDescriptorPoolSize pool_sizes[] = {{VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
                                             {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
                                             {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
                                             {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
                                             {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
                                             {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
                                             {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
                                             {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
                                             {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
                                             {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
                                             {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}};

        VkDescriptorPoolCreateInfo pool_info = {};
        pool_info.sType                      = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.flags                      = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        pool_info.maxSets                    = 1000;
        pool_info.poolSizeCount              = std::size(pool_sizes);
        pool_info.pPoolSizes                 = pool_sizes;

        AssertVkResult(vkCreateDescriptorPool(mContext->Device(), &pool_info, nullptr, &mImguiPool));

        // 2: initialize imgui library

        //this initializes the core structures of imgui
        ImGui::CreateContext();

        //this initializes imgui for SDL
        ImGui_ImplSDL2_InitForVulkan(mContext->Window->GetSdlWindowHandle());

        //this initializes imgui for Vulkan
        ImGui_ImplVulkan_InitInfo init_info = {};
        init_info.Instance                  = mContext->Instance();
        init_info.PhysicalDevice            = mContext->PhysicalDevice();
        init_info.Device                    = mContext->Device();
        init_info.Queue                     = mContext->Queue;
        init_info.DescriptorPool            = mImguiPool;
        init_info.MinImageCount             = 3;
        init_info.ImageCount                = 3;
        init_info.MSAASamples               = VK_SAMPLE_COUNT_1_BIT;

        ImGui_ImplVulkan_Init(&init_info, mRenderPass);

        //execute a gpu command to upload imgui font textures
        core::HostSyncCommandBuffer cmdBuf;
        cmdBuf.Create(mContext, VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
        ImGui_ImplVulkan_CreateFontsTexture(cmdBuf.GetCommandBuffer());
        cmdBuf.SubmitAndWait();

        //clear font textures from cpu data
        ImGui_ImplVulkan_DestroyFontUploadObjects();
    }

    void ImguiStage::DestroyFrameBufferAndRenderPass()
    {
        if(mFrameBuffers.size() > 0)
        {
            for(VkFramebuffer frameBuffer : mFrameBuffers)
            {
                vkDestroyFramebuffer(mContext->Device(), frameBuffer, nullptr);
            }
            mFrameBuffers.clear();
        }
        if(mRenderPass)
        {
            vkDestroyRenderPass(mContext->Device(), mRenderPass, nullptr);
            mRenderPass = nullptr;
        }
    }

    void ImguiStage::PrepareRenderpass()
    {
        // size + 1 for depth attachment description
        VkAttachmentDescription attachmentDescription{.loadOp         = VK_ATTACHMENT_LOAD_OP_LOAD,
                                                      .storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
                                                      .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                                                      .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                                                      .initialLayout  = VkImageLayout::VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
                                                      .finalLayout    = VkImageLayout::VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL};

        if(!!mTargetImage)
        {
            attachmentDescription.format  = mTargetImage->GetFormat();
            attachmentDescription.samples = mTargetImage->GetSampleCount();
        }
        else
        {
            attachmentDescription.format  = mContext->Swapchain->image_format;
            attachmentDescription.samples = VkSampleCountFlagBits::VK_SAMPLE_COUNT_1_BIT;
        }

        VkAttachmentReference colorAttachmentReference{.attachment = 0, .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

        // Subpass description
        VkSubpassDescription subpass    = {};
        subpass.pipelineBindPoint       = VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount    = 1U;
        subpass.pColorAttachments       = &colorAttachmentReference;
        subpass.pDepthStencilAttachment = nullptr;

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
        renderPassInfo.pAttachments           = &attachmentDescription;
        renderPassInfo.attachmentCount        = 1U;
        renderPassInfo.subpassCount           = 1;
        renderPassInfo.pSubpasses             = &subpass;
        renderPassInfo.dependencyCount        = 2;
        renderPassInfo.pDependencies          = subPassDependencies;
        AssertVkResult(vkCreateRenderPass(mContext->Device(), &renderPassInfo, nullptr, &mRenderPass));

        mFrameBuffers.resize(!!mTargetImage ? 1 : mContext->SwapchainImages.size());

        for(uint32_t i = 0; i < mFrameBuffers.size(); i++)
        {
            VkFramebuffer& frameBuffer    = mFrameBuffers[i];
            VkImageView    attachmentView = nullptr;
            if(!!mTargetImage)
            {
                attachmentView = mTargetImage->GetImageView();
            }
            else
            {
                attachmentView = mContext->SwapchainImages[i].ImageView;
            }


            VkFramebufferCreateInfo fbufCreateInfo = {};
            fbufCreateInfo.sType                   = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            fbufCreateInfo.pNext                   = NULL;
            fbufCreateInfo.renderPass              = mRenderPass;
            fbufCreateInfo.pAttachments            = &attachmentView;
            fbufCreateInfo.attachmentCount         = 1U;
            fbufCreateInfo.width                   = mDomain->GetExtent().width;
            fbufCreateInfo.height                  = mDomain->GetExtent().height;
            fbufCreateInfo.layers                  = 1;
            AssertVkResult(vkCreateFramebuffer(mContext->Device(), &fbufCreateInfo, nullptr, &frameBuffer));
        }
    }

    void ImguiStage::Destroy()
    {
        mOnSdlEvent.Destroy();
        DestroyFrameBufferAndRenderPass();
        if(mImguiPool != nullptr)
        {
            ImGui_ImplVulkan_Shutdown();
            ImGui_ImplSDL2_Shutdown();
            ImGui::DestroyContext();
            vkDestroyDescriptorPool(mContext->Device(), mImguiPool, nullptr);
            mImguiPool = nullptr;
        }
    }

    void ImguiStage::OnResized(VkExtent2D extent)
    {
        DestroyFrameBufferAndRenderPass();
        PrepareRenderpass();
    }

    void ImguiStage::SetBackgroundImage(core::ManagedImage* newTargetImage)
    {
        mTargetImage = newTargetImage;
        DestroyFrameBufferAndRenderPass();
        PrepareRenderpass();
    }

    void ImguiStage::HandleSdlEvent(const osi::EventRawSDL* event)
    {
        ImGui_ImplSDL2_ProcessEvent(&event->Data);
    }


    void ImguiStage::RecordFrame(VkCommandBuffer cmdBuffer, base::FrameRenderInfo& renderInfo)
    {
        VkImage       image       = nullptr;
        VkFramebuffer frameBuffer = nullptr;
        if(!!mTargetImage)
        {
            frameBuffer = mFrameBuffers.front();
            image       = mTargetImage->GetImage();
        }
        else
        {
            uint32_t swapchainIndex = renderInfo.GetInFlightFrame()->GetSwapchainImageIndex();
            frameBuffer             = mFrameBuffers[swapchainIndex];
            image                   = mContext->SwapchainImages[swapchainIndex].Image;
        }

        VkRenderPassBeginInfo renderPassBeginInfo{};
        renderPassBeginInfo.sType             = VkStructureType::VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassBeginInfo.renderPass        = mRenderPass;
        renderPassBeginInfo.framebuffer       = frameBuffer;
        renderPassBeginInfo.renderArea.extent = mDomain->GetExtent();
        // renderPassBeginInfo.clearValueCount   = 1U;
        // renderPassBeginInfo.pClearValues      = &mClearValue;

        core::ImageLayoutCache::Barrier2 barrier{.SrcStageMask  = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
                                                 .SrcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT,
                                                 .DstStageMask  = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                                                 .DstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                                                 .NewLayout     = VkImageLayout::VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL};
        renderInfo.GetImageLayoutCache().CmdBarrier(cmdBuffer, image, barrier);

        vkCmdBeginRenderPass(cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

        // imgui drawing
        {
            ImGui_ImplVulkan_NewFrame();
            ImGui_ImplSDL2_NewFrame();
            ImGui::NewFrame();

            for(auto& subdraw : mWindowDraws)
            {
                subdraw();
            }

            ImGui::Render();
            ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmdBuffer);
        }

        vkCmdEndRenderPass(cmdBuffer);
    }

}  // namespace foray::stages