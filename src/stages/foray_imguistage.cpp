#include "foray_imguistage.hpp"
#include "../core/foray_commandbuffer.hpp"
#include "../core/foray_shadermodule.hpp"
#include "../util/foray_pipelinebuilder.hpp"
//#include "../utility/foray_shaderstagecreateinfos.hpp"

#include <imgui/imgui.h>
#include <imgui/imgui_impl_sdl.h>
#include <imgui/imgui_impl_vulkan.h>


namespace foray::stages {
    // Heavily inspired from Sascha Willems' "deferred" vulkan example
    void ImguiStage::Init(const core::VkContext* context, core::ManagedImage* backgroundImage)
    {
        mContext     = context;
        mTargetImage = backgroundImage;

        if(mTargetImage == nullptr)
            throw Exception("Imgui stage can only init when the background image is set!");

        CreateResolutionDependentComponents();
        CreateFixedSizeComponents();

        // Clear values for all attachments written in the fragment shader
        mClearValues.resize(mColorAttachments.size() + 1);
        for(size_t i = 0; i < mColorAttachments.size(); i++)
        {
            mClearValues[i].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
        }
        mClearValues[mColorAttachments.size()].depthStencil = {1.0f, 0};

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

        AssertVkResult(vkCreateDescriptorPool(context->Device, &pool_info, nullptr, &mImguiPool));

        // 2: initialize imgui library

        //this initializes the core structures of imgui
        ImGui::CreateContext();

        //this initializes imgui for SDL
        ImGui_ImplSDL2_InitForVulkan(mContext->ContextSwapchain.Window.GetSdlWindowHandle());

        //this initializes imgui for Vulkan
        ImGui_ImplVulkan_InitInfo init_info = {};
        init_info.Instance                  = mContext->Instance;
        init_info.PhysicalDevice            = mContext->PhysicalDevice;
        init_info.Device                    = mContext->Device;
        init_info.Queue                     = mContext->QueueGraphics;
        init_info.DescriptorPool            = mImguiPool;
        init_info.MinImageCount             = 3;
        init_info.ImageCount                = 3;
        init_info.MSAASamples               = VK_SAMPLE_COUNT_1_BIT;

        ImGui_ImplVulkan_Init(&init_info, mRenderpass);

        //execute a gpu command to upload imgui font textures
        core::CommandBuffer cmdBuf;
        cmdBuf.Create(context, VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
        ImGui_ImplVulkan_CreateFontsTexture(cmdBuf.GetCommandBuffer());
        cmdBuf.Submit();

        //clear font textures from cpu data
        ImGui_ImplVulkan_DestroyFontUploadObjects();
    }

    void ImguiStage::CreateFixedSizeComponents() {}

    void ImguiStage::CreateResolutionDependentComponents()
    {
        PrepareAttachments();
        PrepareRenderpass();
        BuildCommandBuffer();
    }

    void ImguiStage::DestroyResolutionDependentComponents()
    {
        VkDevice device = mContext->Device;

        if(mFrameBuffer)
        {
            vkDestroyFramebuffer(device, mFrameBuffer, nullptr);
            mFrameBuffer = nullptr;
        }
        if(mRenderpass)
        {
            vkDestroyRenderPass(device, mRenderpass, nullptr);
            mRenderpass = nullptr;
        }
    }

    void ImguiStage::PrepareAttachments()
    {
        static const VkFormat colorFormat    = VK_FORMAT_R16G16B16A16_SFLOAT;
        static const VkFormat geometryFormat = VK_FORMAT_R16G16B16A16_SFLOAT;

        static const VkImageUsageFlags imageUsageFlags =
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

        VkExtent3D               extent                = {mContext->Swapchain.extent.width, mContext->Swapchain.extent.height, 1};
        VmaMemoryUsage           memoryUsage           = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
        VmaAllocationCreateFlags allocationCreateFlags = 0;
        VkImageLayout            intialLayout          = VK_IMAGE_LAYOUT_UNDEFINED;
        VkImageAspectFlags       aspectMask            = VK_IMAGE_ASPECT_COLOR_BIT;

        mColorAttachments.clear();
        mColorAttachments.push_back(mTargetImage);
    }

    void ImguiStage::PrepareRenderpass()
    {
        // size + 1 for depth attachment description
        std::vector<VkAttachmentDescription> attachmentDescriptions(mColorAttachments.size());
        std::vector<VkAttachmentReference>   colorAttachmentReferences(mColorAttachments.size());
        std::vector<VkImageView>             attachmentViews(attachmentDescriptions.size());

        for(uint32_t i = 0; i < mColorAttachments.size(); i++)
        {
            auto& colorAttachment                    = mColorAttachments[i];
            attachmentDescriptions[i].samples        = colorAttachment->GetSampleCount();
            attachmentDescriptions[i].loadOp         = VK_ATTACHMENT_LOAD_OP_LOAD;
            attachmentDescriptions[i].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
            attachmentDescriptions[i].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachmentDescriptions[i].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachmentDescriptions[i].initialLayout  = VK_IMAGE_LAYOUT_GENERAL;
            attachmentDescriptions[i].finalLayout    = VK_IMAGE_LAYOUT_GENERAL;
            attachmentDescriptions[i].format         = colorAttachment->GetFormat();

            colorAttachmentReferences[i] = {i, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
            attachmentViews[i]           = colorAttachment->GetImageView();
        }

        // Subpass description
        VkSubpassDescription subpass    = {};
        subpass.pipelineBindPoint       = VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount    = colorAttachmentReferences.size();
        subpass.pColorAttachments       = colorAttachmentReferences.data();
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
        renderPassInfo.pAttachments           = attachmentDescriptions.data();
        renderPassInfo.attachmentCount        = static_cast<uint32_t>(attachmentDescriptions.size());
        renderPassInfo.subpassCount           = 1;
        renderPassInfo.pSubpasses             = &subpass;
        renderPassInfo.dependencyCount        = 2;
        renderPassInfo.pDependencies          = subPassDependencies;
        AssertVkResult(vkCreateRenderPass(mContext->Device, &renderPassInfo, nullptr, &mRenderpass));

        VkFramebufferCreateInfo fbufCreateInfo = {};
        fbufCreateInfo.sType                   = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fbufCreateInfo.pNext                   = NULL;
        fbufCreateInfo.renderPass              = mRenderpass;
        fbufCreateInfo.pAttachments            = attachmentViews.data();
        fbufCreateInfo.attachmentCount         = static_cast<uint32_t>(attachmentViews.size());
        fbufCreateInfo.width                   = mContext->Swapchain.extent.width;
        fbufCreateInfo.height                  = mContext->Swapchain.extent.height;
        fbufCreateInfo.layers                  = 1;
        AssertVkResult(vkCreateFramebuffer(mContext->Device, &fbufCreateInfo, nullptr, &mFrameBuffer));
    }

    void ImguiStage::Destroy()
    {
        if(mImguiPool != nullptr)
        {
            ImGui_ImplVulkan_Shutdown();
            ImGui_ImplSDL2_Shutdown();
            ImGui::DestroyContext();
            RasterizedRenderStage::Destroy();
            vkDestroyDescriptorPool(mContext->Device.device, mImguiPool, nullptr);
            mImguiPool = nullptr;
        }
        RasterizedRenderStage::Destroy();
    }

    void ImguiStage::OnResized(const VkExtent2D& extent, core::ManagedImage* newTargetImage)
    {
        mTargetImage = newTargetImage;
        RasterizedRenderStage::OnResized(extent);
    }

    void ImguiStage::SetTargetImage(core::ManagedImage* newTargetImage)
    {
        mTargetImage = newTargetImage;
        DestroyResolutionDependentComponents();
        CreateResolutionDependentComponents();
    }

    void ImguiStage::ProcessSdlEvent(const SDL_Event* sdlEvent)
    {
        ImGui_ImplSDL2_ProcessEvent(sdlEvent);
    }


    void ImguiStage::RecordFrame(base::FrameRenderInfo& renderInfo)
    {
        VkRenderPassBeginInfo renderPassBeginInfo{};
        renderPassBeginInfo.sType             = VkStructureType::VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassBeginInfo.renderPass        = mRenderpass;
        renderPassBeginInfo.framebuffer       = mFrameBuffer;
        renderPassBeginInfo.renderArea.extent = mContext->Swapchain.extent;
        renderPassBeginInfo.clearValueCount   = static_cast<uint32_t>(mClearValues.size());
        renderPassBeginInfo.pClearValues      = mClearValues.data();

        VkCommandBuffer commandBuffer = renderInfo.GetCommandBuffer();
        vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

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
            ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
        }

        vkCmdEndRenderPass(commandBuffer);
    }

}  // namespace foray::stages