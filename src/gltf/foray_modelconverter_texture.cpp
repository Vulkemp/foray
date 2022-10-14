#include "../core/foray_commandbuffer.hpp"
#include "../scene/globalcomponents/foray_texturestore.hpp"
#include "../util/foray_imageloader.hpp"
#include "foray_modelconverter.hpp"
#include <mutex>
#include <spdlog/fmt/fmt.h>

namespace foray::gltf {

    namespace impl {
        /// @brief Used in the lambda method executing
        struct MultithreadLambdaArgs
        {
            /// @brief Gltf Model to load textures of
            tinygltf::Model& GltfModel;
            /// @brief Texture store to give textures to
            scene::TextureStore& Textures;
            /// @brief Base directory to look for textures relative to
            std::string BaseDir;
            /// @brief Context used for GPU stuff
            const core::VkContext* Context;
            /// @brief This threads index
            int32_t ThreadIndex;
            /// @brief Total count of threads used for loading
            int32_t ThreadCount;
            /// @brief Base index for storing textures into the texture store
            int32_t BaseTexIndex;
            /// @brief Mutex for assuring the single threaded section is accessed by one thread only
            std::mutex& SingleThreadSectionMutex;
            /// @brief Vector
            uint8_t& Done;
        };

        /// @brief Represents a single thread action for multithreaded image loading
        void lLoadTexturesThreadInstance(MultithreadLambdaArgs args)
        {
            for(int32_t texIndex = args.ThreadIndex; texIndex < args.GltfModel.textures.size(); texIndex += args.ThreadCount)
            {
                try
                {
                    const auto& gltfTexture = args.GltfModel.textures[texIndex];
                    const auto& gltfImage   = args.GltfModel.images[gltfTexture.source];

                    std::string textureName;
                    textureName = gltfTexture.name;
                    if(!textureName.size())
                    {
                        textureName = gltfImage.name;
                    }
                    if(!textureName.size())
                    {
                        textureName = fmt::format("Texture #{}", texIndex);
                    }

                    logger()->debug("Model Load: Processing texture #{} \"{}\" on Thread {}/{}", texIndex, textureName, args.ThreadIndex, args.ThreadCount);

                    scene::SampledTexture& sampledTexture = args.Textures.GetTextures()[args.BaseTexIndex + texIndex];
                    sampledTexture.Image                  = std::make_unique<core::ManagedImage>();

                    const unsigned char* buffer     = nullptr;
                    VkDeviceSize         bufferSize = 0;

                    util::ImageLoader<VkFormat::VK_FORMAT_R8G8B8A8_UNORM> imageLoader;

                    if(!imageLoader.Init(args.BaseDir + "/" + gltfImage.uri))
                    {
                        logger()->warn("ImageLoad Init failed for gltfImage \"{}\"", textureName);
                        continue;
                    }
                    if(!imageLoader.Load())
                    {
                        logger()->warn("ImageLoad load failed for gltfImage \"{}\"", textureName);
                        continue;
                    }

                    {  // Any GPU action for the moment is single threaded
                        std::lock_guard<std::mutex> lock(args.SingleThreadSectionMutex);


                        VkExtent2D extent        = imageLoader.GetInfo().Extent;
                        uint32_t   mipLevelCount = (uint32_t)(floorf(log2f(std::max(extent.width, extent.height))));

                        core::ManagedImage::CreateInfo imageCI;
                        imageCI.AllocCI.usage = VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

                        imageCI.ImageCI.imageType     = VK_IMAGE_TYPE_2D;
                        imageCI.ImageCI.format        = VkFormat::VK_FORMAT_R8G8B8A8_UNORM;
                        imageCI.ImageCI.mipLevels     = mipLevelCount;
                        imageCI.ImageCI.arrayLayers   = 1;
                        imageCI.ImageCI.samples       = VK_SAMPLE_COUNT_1_BIT;
                        imageCI.ImageCI.tiling        = VK_IMAGE_TILING_OPTIMAL;
                        imageCI.ImageCI.usage         = VK_IMAGE_USAGE_SAMPLED_BIT;
                        imageCI.ImageCI.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
                        imageCI.ImageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                        imageCI.ImageCI.extent        = VkExtent3D{.width = extent.width, .height = extent.height, .depth = 1};
                        imageCI.ImageCI.usage         = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

                        imageCI.ImageViewCI.viewType                    = VK_IMAGE_VIEW_TYPE_2D;
                        imageCI.ImageViewCI.format                      = VkFormat::VK_FORMAT_R8G8B8A8_UNORM;
                        imageCI.ImageViewCI.components                  = {VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A};
                        imageCI.ImageViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                        imageCI.ImageViewCI.subresourceRange.layerCount = 1;
                        imageCI.ImageViewCI.subresourceRange.levelCount = mipLevelCount;
                        imageCI.Name                                    = textureName;

                        imageLoader.InitManagedImage(args.Context, sampledTexture.Image.get(), imageCI, VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

                        // sampledTexture.Image->Create(mContext, imageCI);
                        // sampledTexture.Image->WriteDeviceLocalData(buffer, bufferSize, VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

                        core::HostCommandBuffer cmdBuf;
                        cmdBuf.Create(args.Context, VkCommandBufferLevel::VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

                        for(int32_t i = 0; i < mipLevelCount - 1; i++)
                        {
                            uint32_t sourceMipLevel = (uint32_t)i;
                            uint32_t destMipLevel   = sourceMipLevel + 1;

                            // Step #1 Transition dest miplevel to transfer dst optimal
                            VkImageSubresourceRange mipSubRange = {};
                            mipSubRange.aspectMask              = VK_IMAGE_ASPECT_COLOR_BIT;
                            mipSubRange.baseMipLevel            = destMipLevel;
                            mipSubRange.levelCount              = 1;
                            mipSubRange.layerCount              = 1;

                            core::ManagedImage::LayoutTransitionInfo layoutTransition;
                            layoutTransition.CommandBuffer        = cmdBuf.GetCommandBuffer();
                            layoutTransition.OldImageLayout       = VK_IMAGE_LAYOUT_UNDEFINED;
                            layoutTransition.NewImageLayout       = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                            layoutTransition.BarrierSrcAccessMask = 0;
                            layoutTransition.BarrierDstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                            layoutTransition.SubresourceRange     = mipSubRange;
                            layoutTransition.SrcStage             = VkPipelineStageFlagBits::VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
                            layoutTransition.DstStage             = VkPipelineStageFlagBits::VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

                            sampledTexture.Image->TransitionLayout(layoutTransition);

                            VkOffset3D  srcArea{.x = (int32_t)extent.width >> i, .y = (int32_t)extent.height >> i, .z = 1};
                            VkOffset3D  dstArea{.x = (int32_t)extent.width >> (i + 1), .y = (int32_t)extent.height >> (i + 1), .z = 1};
                            VkImageBlit blit{.srcSubresource =
                                                 VkImageSubresourceLayers{
                                                     .aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT, .mipLevel = (uint32_t)i, .baseArrayLayer = 0, .layerCount = 1},
                                             .dstSubresource = VkImageSubresourceLayers{.aspectMask     = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT,
                                                                                        .mipLevel       = (uint32_t)i + 1,
                                                                                        .baseArrayLayer = 0,
                                                                                        .layerCount     = 1}};
                            blit.srcOffsets[1] = srcArea;
                            blit.dstOffsets[1] = dstArea;
                            vkCmdBlitImage(cmdBuf.GetCommandBuffer(), sampledTexture.Image->GetImage(), VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                           sampledTexture.Image->GetImage(), VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VkFilter::VK_FILTER_LINEAR);

                            // Step #3 Transition dest miplevel to transfer src optimal
                            layoutTransition.OldImageLayout       = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                            layoutTransition.NewImageLayout       = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                            layoutTransition.BarrierSrcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                            layoutTransition.BarrierDstAccessMask = 0;

                            sampledTexture.Image->TransitionLayout(layoutTransition);
                        }

                        {
                            // All mip levels are transfer src optimal after mip creation, fix it

                            VkImageSubresourceRange mipSubRange = {
                                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .baseMipLevel = 0, .levelCount = VK_REMAINING_MIP_LEVELS, .layerCount = VK_REMAINING_ARRAY_LAYERS};
                            core::ManagedImage::LayoutTransitionInfo layoutTransition;
                            layoutTransition.CommandBuffer        = cmdBuf.GetCommandBuffer();
                            layoutTransition.OldImageLayout       = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                            layoutTransition.NewImageLayout       = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                            layoutTransition.BarrierSrcAccessMask = VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
                            layoutTransition.BarrierDstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
                            layoutTransition.SubresourceRange     = mipSubRange;
                            layoutTransition.SrcStage             = VkPipelineStageFlagBits::VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
                            layoutTransition.DstStage             = VkPipelineStageFlagBits::VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

                            sampledTexture.Image->TransitionLayout(layoutTransition);
                        }

                        cmdBuf.Submit();

                        VkSamplerCreateInfo samplerCI{.sType                   = VkStructureType::VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
                                                      .magFilter               = VkFilter::VK_FILTER_LINEAR,
                                                      .minFilter               = VkFilter::VK_FILTER_LINEAR,
                                                      .addressModeU            = VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_REPEAT,
                                                      .addressModeV            = VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_REPEAT,
                                                      .addressModeW            = VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_REPEAT,
                                                      .mipLodBias              = 0.5f,
                                                      .anisotropyEnable        = VK_TRUE,
                                                      .maxAnisotropy           = 4,
                                                      .compareEnable           = VK_FALSE,
                                                      .compareOp               = {},
                                                      .minLod                  = 0,
                                                      .maxLod                  = VK_LOD_CLAMP_NONE,
                                                      .borderColor             = {},
                                                      .unnormalizedCoordinates = VK_FALSE};
                        if(gltfTexture.sampler >= 0)
                        {
                            ModelConverter::sTranslateSampler(args.GltfModel.samplers[gltfTexture.sampler], samplerCI);
                        }

                        sampledTexture.Sampler = args.Textures.GetOrCreateSampler(samplerCI);
                    }
                }
                catch(const std::exception& ex)
                {
                    logger()->error("Thread #{} exception: {}", args.ThreadIndex, ex.what());
                }
            }

            {
                std::lock_guard<std::mutex> lock(args.SingleThreadSectionMutex);
                args.Done = (uint8_t) true;
            }
        }
    }  // namespace impl

    void ModelConverter::LoadTextures()
    {
        using namespace impl;

        int32_t                  threadCount = (int32_t)std::min(std::thread::hardware_concurrency(), (uint32_t)mGltfModel.textures.size());
        std::vector<std::thread> threads(threadCount);
        std::vector<uint8_t>     done(threadCount);  // Not a vector<bool>, because these are a specialisation in form of a bit vector, which disallows references!

        int32_t baseTexIndex = (int32_t)mTextures.GetTextures().size();
        mTextures.GetTextures().resize(mTextures.GetTextures().size() + mGltfModel.textures.size());

        std::mutex singleThreadedActionsMutex;

        for(int32_t threadIndex = 0; threadIndex < threadCount; threadIndex++)
        {
            MultithreadLambdaArgs args{.GltfModel                = mGltfModel,
                                       .Textures                 = mTextures,
                                       .BaseDir                  = mUtf8Dir,
                                       .Context                  = mContext,
                                       .ThreadIndex              = threadIndex,
                                       .ThreadCount              = threadCount,
                                       .BaseTexIndex             = baseTexIndex,
                                       .SingleThreadSectionMutex = singleThreadedActionsMutex,
                                       .Done                     = done[threadIndex]};
            auto&                 thread = threads[threadIndex];

            thread = std::thread([args]() { lLoadTexturesThreadInstance(args); });
            thread.detach();
        }

        // Wait in main thread until all image loading operations have succeeded
        while(true)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            bool alldone = true;
            {
                std::lock_guard<std::mutex> lock(singleThreadedActionsMutex);
                for(int32_t threadIndex = 0; alldone && threadIndex < threadCount; threadIndex++)
                {
                    alldone &= !!(done[threadIndex]);
                }
            }
            if(alldone)
            {
                break;
            }
        }

        // join all threads
        for(int32_t threadIndex = 0; threadIndex < threadCount; threadIndex++)
        {
            auto& thread = threads[threadIndex];
            if(thread.joinable())
            {
                thread.join();
            }
        }
    }

    void ModelConverter::sTranslateSampler(const tinygltf::Sampler& tinygltfSampler, VkSamplerCreateInfo& outsamplerCI)
    {
        // https://www.khronos.org/registry/glTF/specs/2.0/glTF-2.0.html#reference-sampler
        std::map<int, VkFilter>             filterMap({{9728, VkFilter::VK_FILTER_NEAREST}, {9729, VkFilter::VK_FILTER_LINEAR}});
        std::map<int, VkSamplerAddressMode> addressMap({{33071, VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE},
                                                        {33648, VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT},
                                                        {10497, VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_REPEAT}});
        {  // Magfilter
            auto find = filterMap.find(tinygltfSampler.magFilter);
            if(find != filterMap.end())
            {
                outsamplerCI.magFilter = find->second;
            }
        }
        {  // Minfilter
            auto find = filterMap.find(tinygltfSampler.minFilter);
            if(find != filterMap.end())
            {
                outsamplerCI.minFilter = find->second;
            }
        }
        {  // AddressMode U
            auto find = addressMap.find(tinygltfSampler.wrapS);
            if(find != addressMap.end())
            {
                outsamplerCI.addressModeU = find->second;
            }
        }
        {  // AddressMode V
            auto find = addressMap.find(tinygltfSampler.wrapT);
            if(find != addressMap.end())
            {
                outsamplerCI.addressModeV = find->second;
            }
        }
    }

}  // namespace foray::gltf