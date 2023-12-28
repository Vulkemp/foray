#include "configurablerasterstage.hpp"
#include "../core/shadermanager.hpp"
#include "../scene/geo.hpp"
#include "../scene/globalcomponents/cameramanager.hpp"
#include "../scene/globalcomponents/drawmanager.hpp"
#include "../scene/globalcomponents/materialmanager.hpp"
#include "../scene/globalcomponents/texturemanager.hpp"
#include "../scene/scene.hpp"
#include "../util/shaderstagecreateinfos.hpp"
#include <vulkan/vulkan_format_traits.hpp>

namespace foray::stages {

    // clang-format off
    const ConfigurableRasterStage::OutputRecipe ConfigurableRasterStage::Templates::WorldPos =  
        {.FragmentInputFlags = (uint32_t)FragmentInputFlagBits::WORLDPOS,
         .Type               = FragmentOutputType::VEC4,
         .ImageFormat        = vk::Format::VK_FORMAT_R16G16B16A16_SFLOAT,
         .Result             = "WorldPos,0"};

    const ConfigurableRasterStage::OutputRecipe ConfigurableRasterStage::Templates::WorldNormal = 
        {.FragmentInputFlags = (uint32_t)FragmentInputFlagBits::UV | (uint32_t)FragmentInputFlagBits::NORMAL | (uint32_t)FragmentInputFlagBits::TANGENT,
         .BuiltInFeaturesFlags = (uint32_t)BuiltInFeaturesFlagBits::NORMALMAPPING,
         .Type                 = FragmentOutputType::VEC4,
         .ImageFormat          = vk::Format::VK_FORMAT_R8G8B8A8_SNORM,
         .Result               = "normalMapped,0"};

    const ConfigurableRasterStage::OutputRecipe ConfigurableRasterStage::Templates::Albedo = 
        {.FragmentInputFlags = (uint32_t)FragmentInputFlagBits::UV,
         .BuiltInFeaturesFlags = (uint32_t)BuiltInFeaturesFlagBits::MATERIALPROBE,
         .Type                 = FragmentOutputType::VEC4,
         .ImageFormat          = vk::Format::VK_FORMAT_R8G8B8A8_SRGB,
         .Result               = "probe.BaseColor.rgb,1"};

    const ConfigurableRasterStage::OutputRecipe ConfigurableRasterStage::Templates::MaterialId = 
        {.Type = FragmentOutputType::INT, 
         .ImageFormat = vk::Format::VK_FORMAT_R16_SINT, 
         .ClearValue         = {{-1}},
         .Result = "PushConstant.MaterialIndex"};

    const ConfigurableRasterStage::OutputRecipe ConfigurableRasterStage::Templates::MeshInstanceId = 
        {.FragmentInputFlags = (uint32_t)FragmentInputFlagBits::MESHID,
         .Type               = FragmentOutputType::INT,
         .ImageFormat        = vk::Format::VK_FORMAT_R16_SINT,
         .ClearValue         = {{-1}},
         .Result             = "MeshInstanceId"};

    const ConfigurableRasterStage::OutputRecipe ConfigurableRasterStage::Templates::UV = 
        {.FragmentInputFlags = (uint32_t)FragmentInputFlagBits::UV,
         .Type               = FragmentOutputType::VEC2,
         .ImageFormat        = vk::Format::VK_FORMAT_R16G16_SFLOAT,
         .Result             = "UV"};

    const ConfigurableRasterStage::OutputRecipe ConfigurableRasterStage::Templates::ScreenMotion = 
        {.FragmentInputFlags = (uint32_t)FragmentInputFlagBits::DEVICEPOS | (uint32_t)FragmentInputFlagBits::DEVICEPOSOLD,
         .Type               = FragmentOutputType::VEC2,
         .ImageFormat        = vk::Format::VK_FORMAT_R16G16_SFLOAT,
         .Result             = "((DevicePosOld.xy / DevicePosOld.w) - (DevicePos.xy / DevicePos.w)) * 0.5"};

    const ConfigurableRasterStage::OutputRecipe ConfigurableRasterStage::Templates::WorldMotion = 
        {.FragmentInputFlags = (uint32_t)FragmentInputFlagBits::WORLDPOS | (uint32_t)FragmentInputFlagBits::WORLDPOSOLD,
         .Type               = FragmentOutputType::VEC4,
         .ImageFormat        = vk::Format::VK_FORMAT_R16G16B16A16_SFLOAT,
         .Result             = "WorldPosOld - WorldPos, 0.f"};

    const ConfigurableRasterStage::OutputRecipe ConfigurableRasterStage::Templates::DepthAndDerivative = 
        {.FragmentInputFlags = (uint32_t)FragmentInputFlagBits::DEVICEPOS,
         .Type               = FragmentOutputType::VEC2,
         .ImageFormat        = vk::Format::VK_FORMAT_R16G16_SFLOAT,
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

    ConfigurableRasterStage::Builder& ConfigurableRasterStage::Builder::EnableBuiltInFeature(BuiltInFeaturesFlagBits feature)
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

    ConfigurableRasterStage::Builder& ConfigurableRasterStage::Builder::AddOutput(std::string_view name, const OutputRecipe& recipe)
    {
        std::string keycopy(name);
        FORAY_ASSERTFMT(mOutputMap.size() < MAX_OUTPUT_COUNT, "Can not exceed maximum output count of {}", MAX_OUTPUT_COUNT);
        FORAY_ASSERTFMT(!mOutputMap.contains(keycopy), "Raster stage already configured with an output named \"{}\"", name);
        mOutputMap[keycopy] = recipe;
        return *this;
    }
    const ConfigurableRasterStage::OutputRecipe& ConfigurableRasterStage::Builder::GetOutputRecipe(std::string_view name) const
    {
        std::string               keycopy(name);
        OutputMap::const_iterator iter = mOutputMap.find(keycopy);
        if(iter != mOutputMap.cend())
        {
            return iter->second;
        }
        FORAY_THROWFMT("CGBuffer builder does not contain output \"{}\"!", name);
    }
    ConfigurableRasterStage::OutputRecipe& ConfigurableRasterStage::Builder::GetOutputRecipe(std::string_view name)
    {
        std::string         keycopy(name);
        OutputMap::iterator iter = mOutputMap.find(keycopy);
        if(iter != mOutputMap.end())
        {
            return iter->second;
        }
        FORAY_THROWFMT("CGBuffer builder does not contain output \"{}\"!", name);
    }
    uint32_t ConfigurableRasterStage::GetFragmentOutputComponentCount(FragmentOutputType type)
    {
        switch(type)
        {
            case FragmentOutputType::FLOAT:
            case FragmentOutputType::INT:
            case FragmentOutputType::UINT:
                return 1u;
            case FragmentOutputType::VEC2:
            case FragmentOutputType::IVEC2:
            case FragmentOutputType::UVEC2:
                return 2u;
            case FragmentOutputType::VEC3:
            case FragmentOutputType::IVEC3:
            case FragmentOutputType::UVEC3:
                return 3u;
            case FragmentOutputType::VEC4:
            case FragmentOutputType::IVEC4:
            case FragmentOutputType::UVEC4:
                return 4u;
            default:
                return 0u;
        }
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


    ConfigurableRasterStage::ConfigurableRasterStage(
        core::Context* context, const Builder& builder, scene::Scene* scene, RenderDomain* domain, int32_t resizeOrder, std::string_view name)
        : RasterizedRenderStage(context, domain, resizeOrder)
    {
        mScene = scene;
        mName  = std::string(name);

        mBuiltInFeaturesFlagsGlobal = builder.GetBuiltInFeaturesFlagsGlobal();
        mInterfaceFlagsGlobal       = builder.GetInterfaceFlagsGlobal();

        for(const auto& kvp : builder.GetOutputMap())
        {
            Heap<Output> output(kvp.first, kvp.second);
            mOutputList.push_back(output.Get());
            mOutputMap[kvp.first] = output;
        }

        CheckDeviceColorAttachmentCount();
        CreateOutputs(mDomain->GetExtent());
        CreateRenderPass();
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
            Local<core::Image>& image  = pair.second->Image;
            OutputRecipe&              recipe = pair.second->Recipe;
            std::string_view           name   = pair.second->Name;

            core::Image::CreateInfo ci(vk::ImageUsageFlagBits::VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | vk::ImageUsageFlagBits::eSampled
                                                  | /*vk::ImageUsageFlagBits::eStorage |*/ vk::ImageUsageFlagBits::eTransferSrc,
                                              recipe.ImageFormat, size, name);
            image.New(mContext, ci);
            std::string keycopy(name);
            mImageOutputs[keycopy] = image.Get();
        }
        mDepthOutputName = fmt::format("{}.Depth", mName);
        VkImageUsageFlags depthUsage =
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | eSampled | eTransferSrc;
        core::Image::CreateInfo ci(depthUsage, VK_FORMAT_D32_SFLOAT, size, mDepthOutputName);
        ci.ImageViewCI.subresourceRange.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_DEPTH_BIT;
        mDepthImage.New(mContext, ci);
        mImageOutputs[mDepthOutputName] = mDepthImage.Get();
    }

    void ConfigurableRasterStage::CreateRenderPass()
    {
        for(Output* output : mOutputList)
        {
            mRenderAttachments.AddAttachmentCleared(output->Image.Get(), vk::ImageLayout::VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, output->Recipe.ClearValue);
        }
        mRenderAttachments.SetDepthAttachmentCleared(mDepthImage.Get(), vk::ImageLayout::VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VkClearDepthStencilValue{1.f});
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
        mDescriptorSet.CreateOrUpdate(mContext, fmt::format("{}.DescriptorSet", mName));
    }

    void ConfigurableRasterStage::CreatePipelineLayout()
    {
        util::PipelineLayout::Builder builder;
        builder.AddDescriptorSetLayout(mDescriptorSet.GetLayout());
        builder.AddPushConstantRange<scene::DrawPushConstant>(vk::ShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT | vk::ShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT);
        mPipelineLayout.New(mContext, builder);
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
        mShaderKeys[0] = mContext->ShaderMan->CompileAndLoadShader(FORAY_SHADER_DIR "/configurablerasterstage/crs.vert", mVertexShaderModule, shaderConfig);
        mShaderKeys[1] = mContext->ShaderMan->CompileAndLoadShader(FORAY_SHADER_DIR "/configurablerasterstage/crs.frag", mFragmentShaderModule, shaderConfig);
    }

    void ConfigurableRasterStage::CreatePipeline()
    {
        // vertex layout
        util::RasterPipeline::Builder builder;
        builder.Default_SceneDrawing(mVertexShaderModule->GetShaderStageCi(vk::ShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT),
                                     mFragmentShaderModule->GetShaderStageCi(vk::ShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT), &mRenderAttachments, mDomain->GetExtent(),
                                     mPipelineLayout->GetPipelineLayout(), util::RasterPipeline::BuiltinDepthInit::Normal);
        // builder.SetCullModeFlags(VkCullModeFlagBits::VK_CULL_MODE_NONE);

        mPipeline.New(mContext, builder);
    }

    void ConfigurableRasterStage::RecordFrame(VkCommandBuffer cmdBuffer, base::FrameRenderInfo& renderInfo)
    {
        {  // Synchronize buffer access
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

        VkExtent2D extent{mDomain->GetExtent()};

        // Begin rendering
        mRenderAttachments.CmdBeginRendering(cmdBuffer, extent, renderInfo.GetImageLayoutCache(), renderInfo.GetInFlightFrame()->GetSwapchainImageIndex());

        {  // Bind pipeline
            vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipeline->GetPipeline());

            // update dynamic state
            VkRect2D   scissor{{}, extent};
            VkViewport viewport{.x = 0.f, .y = 0.f, .width = (fp32_t)extent.width, .height = (fp32_t)extent.height, .minDepth = 0.f, .maxDepth = 1.f};
            vkCmdSetScissor(cmdBuffer, 0u, 1u, &scissor);
            vkCmdSetViewport(cmdBuffer, 0u, 1u, &viewport);
        }

        // Bind descriptorset
        VkDescriptorSet descriptorSet = mDescriptorSet.GetSet();
        vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipelineLayout.GetRef(), 0, 1, &descriptorSet, 0, nullptr);

        // draw scene
        mScene->Draw(renderInfo, mPipelineLayout.GetRef(), cmdBuffer);

        // end renderpass
        vkCmdEndRendering(cmdBuffer);
    }

    void ConfigurableRasterStage::OnResized(VkExtent2D extent)
    {
        for(auto& pair : mOutputMap)
        {
            if(pair.second->Image.Exists())
            {
                pair.second->Image.Resize(extent);
            }
        }
        mDepthImage.Resize(extent);
    }

    ConfigurableRasterStage::~ConfigurableRasterStage() {}

    uint32_t ConfigurableRasterStage::GetDeviceMaxAttachmentCount(vkb::Device* device)
    {
        return device->physical_device.properties.limits.maxColorAttachments;
    }
    uint32_t ConfigurableRasterStage::GetDeviceMaxAttachmentCount(vkb::PhysicalDevice* device)
    {
        return device->properties.limits.maxColorAttachments;
    }

}  // namespace foray::stages