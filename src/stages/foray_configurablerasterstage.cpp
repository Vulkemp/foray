#include "foray_configurablerasterstage.hpp"
#include "../core/foray_shadermanager.hpp"
#include "../scene/foray_geo.hpp"
#include "../scene/foray_scene.hpp"
#include "../scene/globalcomponents/foray_cameramanager.hpp"
#include "../scene/globalcomponents/foray_drawmanager.hpp"
#include "../scene/globalcomponents/foray_materialmanager.hpp"
#include "../scene/globalcomponents/foray_texturemanager.hpp"
#include "../util/foray_pipelinebuilder.hpp"
#include "../util/foray_shaderstagecreateinfos.hpp"

namespace foray::stages {

    // clang-format off
    const ConfigurableRasterStage::OutputRecipe ConfigurableRasterStage::Templates::WorldPos =  
        {.FragmentInputFlags = (uint32_t)FragmentInputFlagBits::WORLDPOS,
         .Type               = FragmentOutputType::VEC4,
         .ImageFormat        = VkFormat::VK_FORMAT_R16G16B16A16_SFLOAT,
         .Result             = "WorldPos,0"};

    const ConfigurableRasterStage::OutputRecipe ConfigurableRasterStage::Templates::WorldNormal = 
        {.FragmentInputFlags = (uint32_t)FragmentInputFlagBits::UV | (uint32_t)FragmentInputFlagBits::NORMAL | (uint32_t)FragmentInputFlagBits::TANGENT,
         .BuiltInFeaturesFlags = (uint32_t)BuiltInFeaturesFlagBits::NORMALMAPPING,
         .Type                 = FragmentOutputType::VEC4,
         .ImageFormat          = VkFormat::VK_FORMAT_R16G16B16A16_SFLOAT,
         .Result               = "normalMapped,0"};

    const ConfigurableRasterStage::OutputRecipe ConfigurableRasterStage::Templates::Albedo = 
        {.FragmentInputFlags = (uint32_t)FragmentInputFlagBits::UV,
         .BuiltInFeaturesFlags = (uint32_t)BuiltInFeaturesFlagBits::MATERIALPROBE,
         .Type                 = FragmentOutputType::VEC4,
         .ImageFormat          = VkFormat::VK_FORMAT_R16G16B16A16_SFLOAT,
         .Result               = "probe.BaseColor.rgb,1"};

    const ConfigurableRasterStage::OutputRecipe ConfigurableRasterStage::Templates::MaterialId = 
        {.Type = FragmentOutputType::INT, 
         .ImageFormat = VkFormat::VK_FORMAT_R32_SINT, 
         .ClearValue         = {{-1}},
         .Result = "PushConstant.MaterialIndex"};

    const ConfigurableRasterStage::OutputRecipe ConfigurableRasterStage::Templates::MeshInstanceId = 
        {.FragmentInputFlags = (uint32_t)FragmentInputFlagBits::MESHID,
         .Type               = FragmentOutputType::INT,
         .ImageFormat        = VkFormat::VK_FORMAT_R32_SINT,
         .ClearValue         = {{-1}},
         .Result             = "MeshInstanceId"};

    const ConfigurableRasterStage::OutputRecipe ConfigurableRasterStage::Templates::UV = 
        {.FragmentInputFlags = (uint32_t)FragmentInputFlagBits::UV,
         .Type               = FragmentOutputType::VEC2,
         .ImageFormat        = VkFormat::VK_FORMAT_R16G16_SFLOAT,
         .Result             = "UV"};

    const ConfigurableRasterStage::OutputRecipe ConfigurableRasterStage::Templates::ScreenMotion = 
        {.FragmentInputFlags = (uint32_t)FragmentInputFlagBits::DEVICEPOS | (uint32_t)FragmentInputFlagBits::DEVICEPOSOLD,
         .Type               = FragmentOutputType::VEC2,
         .ImageFormat        = VkFormat::VK_FORMAT_R16G16_SFLOAT,
         .Result             = "((DevicePosOld.xy / DevicePosOld.w) - (DevicePos.xy / DevicePos.w)) * 0.5"};

    const ConfigurableRasterStage::OutputRecipe ConfigurableRasterStage::Templates::WorldMotion = 
        {.FragmentInputFlags = (uint32_t)FragmentInputFlagBits::WORLDPOS | (uint32_t)FragmentInputFlagBits::WORLDPOSOLD,
         .Type               = FragmentOutputType::VEC4,
         .ImageFormat        = VkFormat::VK_FORMAT_R16G16B16A16_SFLOAT,
         .Result             = "WorldPosOld - WorldPos, 0.f"};

    const ConfigurableRasterStage::OutputRecipe ConfigurableRasterStage::Templates::DepthAndDerivative = 
        {.FragmentInputFlags = (uint32_t)FragmentInputFlagBits::DEVICEPOS,
         .Type               = FragmentOutputType::VEC2,
         .ImageFormat        = VkFormat::VK_FORMAT_R16G16_SFLOAT,
         .ClearValue         = {{65504.f, 0.f}},
         .Calculation        = "float linearZ = DevicePos.z * DevicePos.w; float derivative = max(abs(dFdx(linearZ)), abs(dFdy(linearZ)));",
         .Result             = "linearZ, derivative"};
    // clang-format on


    std::string ConfigurableRasterStage::ToString(FragmentInputFlagBits input)
    {
        switch(input)
        {
            case FragmentInputFlagBits::WORLDPOS:
                return "INTERFACE_WORLDPOS";
            case FragmentInputFlagBits::WORLDPOSOLD:
                return "INTERFACE_WORLDPOSOLD";
            case FragmentInputFlagBits::DEVICEPOS:
                return "INTERFACE_DEVICEPOS";
            case FragmentInputFlagBits::DEVICEPOSOLD:
                return "INTERFACE_DEVICEPOSOLD";
            case FragmentInputFlagBits::NORMAL:
                return "INTERFACE_NORMAL";
            case FragmentInputFlagBits::TANGENT:
                return "INTERFACE_TANGENT";
            case FragmentInputFlagBits::UV:
                return "INTERFACE_UV";
            case FragmentInputFlagBits::MESHID:
                return "INTERFACE_MESHID";
            default:
                FORAY_THROWFMT("Unhandled FragmentInputFlagBits value 0x{:x}", (uint32_t)input);
        }
    }
    std::string ConfigurableRasterStage::ToString(BuiltInFeaturesFlagBits feature)
    {
        switch(feature)
        {
            case BuiltInFeaturesFlagBits::MATERIALPROBE:
                return "MATERIALPROBE";
            case BuiltInFeaturesFlagBits::MATERIALPROBEALPHA:
                return "MATERIALPROBEALPHA";
            case BuiltInFeaturesFlagBits::ALPHATEST:
                return "ALPHATEST";
            case BuiltInFeaturesFlagBits::NORMALMAPPING:
                return "NORMALMAPPING";
            default:
                FORAY_THROWFMT("Unhandled BuiltInFeaturesFlagBits value 0x{:x}", (uint32_t)feature);
        }
    }
    std::string ConfigurableRasterStage::ToString(FragmentOutputType type)
    {
        switch(type)
        {
            case FragmentOutputType::FLOAT:
                return "float";
            case FragmentOutputType::INT:
                return "int";
            case FragmentOutputType::UINT:
                return "uint";
            case FragmentOutputType::VEC2:
                return "vec2";
            case FragmentOutputType::VEC3:
                return "vec3";
            case FragmentOutputType::VEC4:
                return "vec4";
            case FragmentOutputType::IVEC2:
                return "ivec2";
            case FragmentOutputType::IVEC3:
                return "ivec3";
            case FragmentOutputType::IVEC4:
                return "ivec4";
            case FragmentOutputType::UVEC2:
                return "uvec2";
            case FragmentOutputType::UVEC3:
                return "uvec3";
            case FragmentOutputType::UVEC4:
                return "uvec4";
            default:
                FORAY_THROWFMT("Unhandled FragmentOutputType value 0x{:x}", (uint32_t)type);
        }
    }

    ConfigurableRasterStage::OutputRecipe& ConfigurableRasterStage::OutputRecipe::AddFragmentInput(FragmentInputFlagBits input)
    {
        FragmentInputFlags |= (uint32_t)input;
        return *this;
    }
    ConfigurableRasterStage::OutputRecipe& ConfigurableRasterStage::OutputRecipe::EnableBuiltInFeature(BuiltInFeaturesFlagBits feature)
    {
        BuiltInFeaturesFlags |= (uint32_t)feature;
        switch(feature)
        {
            case BuiltInFeaturesFlagBits::MATERIALPROBE: {
                AddFragmentInput(FragmentInputFlagBits::UV);
                break;
            }
            case BuiltInFeaturesFlagBits::MATERIALPROBEALPHA: {
                AddFragmentInput(FragmentInputFlagBits::UV);
                break;
            }
            case BuiltInFeaturesFlagBits::ALPHATEST: {
                AddFragmentInput(FragmentInputFlagBits::UV);
                break;
            }
            case BuiltInFeaturesFlagBits::NORMALMAPPING: {
                AddFragmentInput(FragmentInputFlagBits::UV);
                AddFragmentInput(FragmentInputFlagBits::NORMAL);
                AddFragmentInput(FragmentInputFlagBits::TANGENT);
                break;
            }
            default:
                break;
        }
        return *this;
    }

    ConfigurableRasterStage& ConfigurableRasterStage::EnableBuiltInFeature(BuiltInFeaturesFlagBits feature)
    {
        mBuiltInFeaturesFlagsGlobal |= (uint32_t)feature;
        switch(feature)
        {
            case BuiltInFeaturesFlagBits::MATERIALPROBE: {
                mInterfaceFlagsGlobal |= (uint32_t)FragmentInputFlagBits::UV;
                break;
            }
            case BuiltInFeaturesFlagBits::MATERIALPROBEALPHA: {
                mInterfaceFlagsGlobal |= (uint32_t)FragmentInputFlagBits::UV;
                break;
            }
            case BuiltInFeaturesFlagBits::ALPHATEST: {
                mInterfaceFlagsGlobal |= (uint32_t)FragmentInputFlagBits::UV;
                break;
            }
            case BuiltInFeaturesFlagBits::NORMALMAPPING: {
                mInterfaceFlagsGlobal |= (uint32_t)FragmentInputFlagBits::UV;
                mInterfaceFlagsGlobal |= (uint32_t)FragmentInputFlagBits::NORMAL;
                mInterfaceFlagsGlobal |= (uint32_t)FragmentInputFlagBits::TANGENT;
                break;
            }
            default:
                break;
        }
        return *this;
    }

    ConfigurableRasterStage& ConfigurableRasterStage::AddOutput(std::string_view name, const OutputRecipe& recipe)
    {
        std::string keycopy(name);
        FORAY_ASSERTFMT(mOutputMap.size() < MAX_OUTPUT_COUNT, "Can not exceed maximum output count of {}", MAX_OUTPUT_COUNT);
        FORAY_ASSERTFMT(!mOutputMap.contains(keycopy), "Raster stage already configured with an output named \"{}\"", name);
        Assert(!mPipeline, "Must add outputs before building!");
        Heap<Output>& output = mOutputMap[keycopy] = Heap<Output>(name, recipe);
        mOutputList.push_back(output.GetData());
        return *this;
    }
    const ConfigurableRasterStage::OutputRecipe& ConfigurableRasterStage::GetOutputRecipe(std::string_view name) const
    {
        std::string               keycopy(name);
        OutputMap::const_iterator iter = mOutputMap.find(keycopy);
        if(iter != mOutputMap.cend())
        {
            return iter->second->Recipe;
        }
        FORAY_THROWFMT("CGBuffer does not contain output \"{}\"!", name);
    }

    VkAttachmentDescription ConfigurableRasterStage::Output::GetAttachmentDescr() const
    {
        return VkAttachmentDescription{.flags          = 0,
                                       .format         = Image->GetFormat(),
                                       .samples        = Image->GetSampleCount(),
                                       .loadOp         = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_CLEAR,
                                       .storeOp        = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_STORE,
                                       .stencilLoadOp  = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                                       .stencilStoreOp = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_DONT_CARE,
                                       .initialLayout  = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED,
                                       .finalLayout    = VkImageLayout::VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    }

    core::ManagedImage* ConfigurableRasterStage::GetDepthImage()
    {
        return mDepthImage;
    }

    void ConfigurableRasterStage::Build(core::Context* context, scene::Scene* scene, RenderDomain* domain, int32_t resizeOrder, std::string_view name)
    {
        RenderStage::InitCallbacks(context, domain, resizeOrder);
        mScene = scene;
        mName  = std::string(name);

        CheckDeviceColorAttachmentCount();
        CreateOutputs(mDomain->GetExtent());
        CreateRenderPass();
        CreateFrameBuffer();
        SetupDescriptors();
        CreateDescriptorSets();
        CreatePipelineLayout();
        ConfigureAndCompileShaders();
        CreatePipeline();
    }

    void ConfigurableRasterStage::CheckDeviceColorAttachmentCount()
    {
        uint32_t max = mContext->Device->GetPhysicalDevice().properties.limits.maxColorAttachments;
        FORAY_ASSERTFMT(mOutputList.size() <= max,
                        "Physical Device supports max of {} color attachments! As configured requires {}. See VkPhysicalDeviceLimits::maxColorAttachments.", max,
                        mOutputList.size())
    }

    void ConfigurableRasterStage::CreateOutputs(const VkExtent2D& size)
    {
        for(auto& pair : mOutputMap)
        {
            Local<core::ManagedImage>& image  = pair.second->Image;
            OutputRecipe&              recipe = pair.second->Recipe;
            std::string_view           name   = pair.second->Name;

            core::ManagedImage::CreateInfo ci(VkImageUsageFlagBits::VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VkImageUsageFlagBits::VK_IMAGE_USAGE_SAMPLED_BIT
                                                  | VkImageUsageFlagBits::VK_IMAGE_USAGE_STORAGE_BIT | VkImageUsageFlagBits::VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                                              recipe.ImageFormat, size, name);
            image.New(mContext, ci);
            std::string keycopy(name);
            mImageOutputs[keycopy] = image;
        }
        mDepthOutputName = fmt::format("{}.Depth", mName);
        VkImageUsageFlags depthUsage =
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        core::ManagedImage::CreateInfo ci(depthUsage, VK_FORMAT_D32_SFLOAT, size, mDepthOutputName);
        ci.ImageViewCI.subresourceRange.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_DEPTH_BIT;
        mDepthImage.New(mContext, ci);
        mImageOutputs[mDepthOutputName] = mDepthImage;
    }

    void ConfigurableRasterStage::CreateRenderPass()
    {
        std::vector<VkAttachmentReference>   colorAttachmentRefs;
        std::vector<VkAttachmentDescription> attachmentDescr;

        for(uint32_t outLocation = 0; outLocation < mOutputList.size(); outLocation++)
        {
            colorAttachmentRefs.push_back({outLocation, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});
            attachmentDescr.push_back(mOutputList[outLocation]->GetAttachmentDescr());
        }

        uint32_t              depthLocation = mOutputList.size();
        VkAttachmentReference depthAttachmentRef{depthLocation, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
        attachmentDescr.push_back(VkAttachmentDescription{.flags          = 0,
                                                          .format         = mDepthImage->GetFormat(),
                                                          .samples        = mDepthImage->GetSampleCount(),
                                                          .loadOp         = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_CLEAR,
                                                          .storeOp        = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_STORE,
                                                          .stencilLoadOp  = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                                                          .stencilStoreOp = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_DONT_CARE,
                                                          .initialLayout  = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED,
                                                          .finalLayout    = VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL});

        // Subpass description
        VkSubpassDescription subpass    = {};
        subpass.pipelineBindPoint       = VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount    = colorAttachmentRefs.size();
        subpass.pColorAttachments       = colorAttachmentRefs.data();
        subpass.pDepthStencilAttachment = &depthAttachmentRef;

        VkSubpassDependency subPassDependencies[2] = {};
        subPassDependencies[0].srcSubpass          = VK_SUBPASS_EXTERNAL;
        subPassDependencies[0].dstSubpass          = 0;
        subPassDependencies[0].srcStageMask        = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        subPassDependencies[0].dstStageMask =
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        subPassDependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        subPassDependencies[0].dstAccessMask =
            VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        subPassDependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        subPassDependencies[1].srcSubpass = 0;
        subPassDependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
        subPassDependencies[1].srcStageMask =
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        subPassDependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        subPassDependencies[1].srcAccessMask =
            VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        subPassDependencies[1].dstAccessMask   = VK_ACCESS_MEMORY_READ_BIT;
        subPassDependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        VkRenderPassCreateInfo renderPassInfo = {};
        renderPassInfo.sType                  = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.pAttachments           = attachmentDescr.data();
        renderPassInfo.attachmentCount        = (uint32_t)attachmentDescr.size();
        renderPassInfo.subpassCount           = 1;
        renderPassInfo.pSubpasses             = &subpass;
        renderPassInfo.dependencyCount        = 2;
        renderPassInfo.pDependencies          = subPassDependencies;
        AssertVkResult(vkCreateRenderPass(mContext->VkDevice(), &renderPassInfo, nullptr, &mRenderpass));
    }
    void ConfigurableRasterStage::CreateFrameBuffer()
    {
        std::vector<VkImageView> attachmentViews;

        for(uint32_t outLocation = 0; outLocation < mOutputList.size(); outLocation++)
        {
            attachmentViews.push_back(mOutputList[outLocation]->Image->GetImageView());
        }
        attachmentViews.push_back(mDepthImage->GetImageView());

        VkFramebufferCreateInfo fbufCreateInfo = {};
        fbufCreateInfo.sType                   = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fbufCreateInfo.pNext                   = NULL;
        fbufCreateInfo.renderPass              = mRenderpass;
        fbufCreateInfo.pAttachments            = attachmentViews.data();
        fbufCreateInfo.attachmentCount         = (uint32_t)attachmentViews.size();
        fbufCreateInfo.width                   = mDomain->GetExtent().width;
        fbufCreateInfo.height                  = mDomain->GetExtent().height;
        fbufCreateInfo.layers                  = 1;
        AssertVkResult(vkCreateFramebuffer(mContext->VkDevice(), &fbufCreateInfo, nullptr, &mFrameBuffer));
    }

    void ConfigurableRasterStage::SetupDescriptors()
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

    void ConfigurableRasterStage::CreateDescriptorSets()
    {
        mDescriptorSet.Create(mContext, fmt::format("{}.DescriptorSet", mName));
    }

    void ConfigurableRasterStage::CreatePipelineLayout()
    {
        mPipelineLayout.AddDescriptorSetLayout(mDescriptorSet.GetDescriptorSetLayout());
        mPipelineLayout.AddPushConstantRange<scene::DrawPushConstant>(VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT | VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT);
        mPipelineLayout.Build(mContext);
    }

    void ConfigurableRasterStage::ConfigureAndCompileShaders()
    {
        core::ShaderCompilerConfig shaderConfig;

        uint32_t interfaceFlags = mInterfaceFlagsGlobal;
        uint32_t featuresFlags  = mBuiltInFeaturesFlagsGlobal;

        for(uint32_t outLocation = 0; outLocation < mOutputList.size(); outLocation++)
        {
            interfaceFlags |= mOutputList[outLocation]->Recipe.FragmentInputFlags;
            featuresFlags |= mOutputList[outLocation]->Recipe.BuiltInFeaturesFlags;
        }

        // Add interface and feature flags

        for(uint32_t flag = 1; flag < (uint32_t)FragmentInputFlagBits::MAXENUM; flag = flag << 1)
        {
            if((interfaceFlags & flag) > 0)
            {
                shaderConfig.Definitions.push_back(fmt::format("{}=1", ToString((FragmentInputFlagBits)flag)));
            }
        }

        for(uint32_t flag = 1; flag < (uint32_t)BuiltInFeaturesFlagBits::MAXENUM; flag = flag << 1)
        {
            if((featuresFlags & flag) > 0)
            {
                shaderConfig.Definitions.push_back(fmt::format("{}=1", ToString((BuiltInFeaturesFlagBits)flag)));
            }
        }

        for(uint32_t outLocation = 0; outLocation < mOutputList.size(); outLocation++)
        {
            OutputRecipe& recipe = mOutputList[outLocation]->Recipe;
            shaderConfig.Definitions.push_back(fmt::format("OUT_{}=1", outLocation));
            shaderConfig.Definitions.push_back(fmt::format("OUT_{}_TYPE={}", outLocation, ToString(recipe.Type)));
            shaderConfig.Definitions.push_back(fmt::format("OUT_{}_RESULT=\"{}\"", outLocation, recipe.Result));
            shaderConfig.Definitions.push_back(fmt::format("OUT_{}_CALC=\"{}\"", outLocation, recipe.Calculation));
        }

        mShaderKeys.resize(2);
        mShaderKeys[0] = mContext->ShaderMan->CompileShader(FORAY_SHADER_DIR "/configurablerasterstage/crs.vert", mVertexShaderModule, shaderConfig);
        mShaderKeys[1] = mContext->ShaderMan->CompileShader(FORAY_SHADER_DIR "/configurablerasterstage/crs.frag", mFragmentShaderModule, shaderConfig);
    }

    void ConfigurableRasterStage::CreatePipeline()
    {
        util::ShaderStageCreateInfos shaderStageCreateInfos;
        shaderStageCreateInfos.Add(VK_SHADER_STAGE_VERTEX_BIT, *mVertexShaderModule).Add(VK_SHADER_STAGE_FRAGMENT_BIT, *mFragmentShaderModule);

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
            .SetDomain(mDomain)
            .SetPipelineLayout(mPipelineLayout.GetPipelineLayout())
            .SetVertexInputStateBuilder(&vertexInputStateBuilder)
            .SetShaderStageCreateInfos(shaderStageCreateInfos.Get())
            .SetPipelineCache(mContext->PipelineCache)
            .SetRenderPass(mRenderpass)
            .Build();
        // clang-format on
    }

    void ConfigurableRasterStage::RecordFrame(VkCommandBuffer cmdBuffer, base::FrameRenderInfo& renderInfo)
    {
        {
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
            };

            vkCmdPipelineBarrier2(cmdBuffer, &depInfo);
        }

        std::vector<VkClearValue> clearValues(mOutputList.size() + 1);

        for(uint32_t i = 0; i < mOutputList.size(); i++)
        {
            clearValues[i].color = mOutputList[i]->Recipe.ClearValue;
        }
        clearValues.back().depthStencil = VkClearDepthStencilValue{1.f, 0};

        VkRenderPassBeginInfo renderPassBeginInfo{};
        renderPassBeginInfo.sType             = VkStructureType::VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassBeginInfo.renderPass        = mRenderpass;
        renderPassBeginInfo.framebuffer       = mFrameBuffer;
        renderPassBeginInfo.renderArea.extent = mDomain->GetExtent();
        renderPassBeginInfo.clearValueCount   = static_cast<uint32_t>(clearValues.size());
        renderPassBeginInfo.pClearValues      = clearValues.data();

        vkCmdBeginRenderPass(cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

        VkExtent2D renderSize = mDomain->GetExtent();
        VkViewport viewport   = {};
        if(mFlipY)
        {
            viewport = VkViewport{0.f, (float)renderSize.height, (float)renderSize.width, (float)renderSize.height * -1.f, 0.0f, 1.0f};
        }
        else
        {
            viewport = VkViewport{0.f, 0.f, (float)renderSize.width, (float)renderSize.height, 0.0f, 1.0f};
        }
        vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);

        VkRect2D scissor{VkOffset2D{}, VkExtent2D{mDomain->GetExtent()}};
        vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);

        vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipeline);

        VkDescriptorSet descriptorSet = mDescriptorSet.GetDescriptorSet();
        // Instanced object
        vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipelineLayout, 0, 1, &descriptorSet, 0, nullptr);

        mScene->Draw(renderInfo, mPipelineLayout, cmdBuffer);

        vkCmdEndRenderPass(cmdBuffer);

        // The GBuffer determines the images layouts

        for(uint32_t i = 0; i < mOutputList.size(); i++)
        {
            renderInfo.GetImageLayoutCache().Set(mOutputList[i]->Image, VkImageLayout::VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        }
        renderInfo.GetImageLayoutCache().Set(mDepthImage, VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
    }

    void ConfigurableRasterStage::OnResized(VkExtent2D extent)
    {
        if(!!mFrameBuffer)
        {
            vkDestroyFramebuffer(mContext->VkDevice(), mFrameBuffer, nullptr);
            mFrameBuffer = nullptr;
        }

        for(auto& pair : mOutputMap)
        {
            Local<core::ManagedImage>& image = pair.second->Image;
            if(image)
            {
                core::ManagedImage::Resize(image, extent);
            }
        }
        core::ManagedImage::Resize(mDepthImage, extent);

        CreateFrameBuffer();
    }

    ConfigurableRasterStage::~ConfigurableRasterStage()
    {
        VkDevice device = !!mContext ? mContext->VkDevice() : nullptr;
        if(device && mPipeline)
        {
            vkDestroyPipeline(device, mPipeline, nullptr);
        }
        if(device && mFrameBuffer)
        {
            vkDestroyFramebuffer(device, mFrameBuffer, nullptr);
            mFrameBuffer = nullptr;
        }
        if(device && mRenderpass)
        {
            vkDestroyRenderPass(device, mRenderpass, nullptr);
            mRenderpass = nullptr;
        }
    }

    uint32_t ConfigurableRasterStage::GetDeviceMaxAttachmentCount(vkb::Device* device)
    {
        return device->physical_device.properties.limits.maxColorAttachments;
    }
    uint32_t ConfigurableRasterStage::GetDeviceMaxAttachmentCount(vkb::PhysicalDevice* device)
    {
        return device->properties.limits.maxColorAttachments;
    }

}  // namespace foray::stages