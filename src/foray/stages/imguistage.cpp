#include "imguistage.hpp"
#include "../core/commandbuffer.hpp"
#include "../core/shadermodule.hpp"
#include "../osi/osmanager.hpp"
#include "../osi/window.hpp"

#include <imgui/imgui.h>
#include <imgui/imgui_impl_sdl.h>
#include <imgui/imgui_impl_vulkan.h>


namespace foray::stages {
    ImguiStage::ImguiStage(core::Context* context, RenderDomain* domain, core::Image* backgroundImage, int32_t resizeOrder) : RenderStage(context, domain, resizeOrder)
    {
        mTargetImage = backgroundImage;
        mOnSdlEvent.Set(context->OsManager->OnEventRawSDL(), [this](const osi::EventRawSDL* event) { this->HandleSdlEvent(event); });

        Assert(!!mTargetImage, "Imgui stage can only init when the background image is set!");

        PrepareRenderpass();
        InitImgui();
    }

    ImguiStage::ImguiStage(core::Context* context, int32_t resizeOrder) : RenderStage(context, context->WindowSwapchain, resizeOrder)
    {
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

        AssertVkResult(vkCreateDescriptorPool(mContext->VkDevice(), &pool_info, nullptr, &mImguiPool));

        // 2: initialize imgui library

        //this initializes the core structures of imgui
        ImGui::CreateContext();

        //this initializes imgui for SDL
        ImGui_ImplSDL2_InitForVulkan(mContext->WindowSwapchain->GetWindow().GetSdlWindowHandle());

        //this initializes imgui for Vulkan
        ImGui_ImplVulkan_InitInfo init_info = {};
        init_info.Instance                  = mContext->vk::Instance();
        init_info.PhysicalDevice            = mContext->VkPhysicalDevice();
        init_info.Device                    = mContext->VkDevice();
        init_info.Queue                     = mContext->Queue;
        init_info.DescriptorPool            = mImguiPool;
        init_info.MinImageCount             = 3;
        init_info.ImageCount                = 3;
        init_info.MSAASamples               = VK_SAMPLE_COUNT_1_BIT;

        mRenderingCi = mRenderAttachments.MakePipelineRenderingCi();
        ImGui_ImplVulkan_Init(&init_info, nullptr, &mRenderingCi);

        //execute a gpu command to upload imgui font textures
        core::HostSyncCommandBuffer cmdBuf(mContext, VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
        ImGui_ImplVulkan_CreateFontsTexture(cmdBuf.GetCommandBuffer());
        cmdBuf.SubmitAndWait();

        //clear font textures from cpu data
        ImGui_ImplVulkan_DestroyFontUploadObjects();
    }

    void ImguiStage::DestroyFrameBufferAndRenderPass()
    {
    }

    void ImguiStage::PrepareRenderpass()
    {
        if(!!mTargetImage)
        {
            mRenderAttachments.AddAttachmentLoaded(mTargetImage, vk::ImageLayout::VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        }
        else
        {
            mRenderAttachments.AddAttachmentLoaded(mContext->WindowSwapchain, vk::ImageLayout::VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        }
    }

    ImguiStage::~ImguiStage()
    {
        DestroyFrameBufferAndRenderPass();
        if(mImguiPool != nullptr)
        {
            ImGui_ImplVulkan_Shutdown();
            ImGui_ImplSDL2_Shutdown();
            ImGui::DestroyContext();
            vkDestroyDescriptorPool(mContext->VkDevice(), mImguiPool, nullptr);
            mImguiPool = nullptr;
        }
    }

    void ImguiStage::OnResized(VkExtent2D extent)
    {
    }

    void ImguiStage::SetBackgroundImage(core::Image* newTargetImage)
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
        vk::Image  image          = nullptr;
        uint32_t swapchainIndex = renderInfo.GetInFlightFrame()->GetSwapchainImageIndex();

        if(!!mTargetImage)
        {
            image = mTargetImage->GetImage();
        }
        else
        {
            image = mContext->WindowSwapchain->GetSwapchainImages()[swapchainIndex].Image;
        }

        core::ImageLayoutCache::Barrier2 barrier{.SrcStageMask  = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
                                                 .SrcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT,
                                                 .DstStageMask  = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                                                 .DstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                                                 .NewLayout     = vk::ImageLayout::VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL};
        renderInfo.GetImageLayoutCache().CmdBarrier(cmdBuffer, image, barrier);

        mRenderAttachments.CmdBeginRendering(cmdBuffer, mDomain->GetExtent(), renderInfo.GetImageLayoutCache(), swapchainIndex);

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

        vkCmdEndRendering(cmdBuffer);
    }

}  // namespace foray::stages