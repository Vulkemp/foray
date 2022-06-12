#include "../memory/hsk_singletimecommandbuffer.hpp"
#include "../scenegraph/globalcomponents/hsk_texturestore.hpp"
#include "hsk_modelconverter.hpp"
#include <spdlog/fmt/fmt.h>

namespace hsk {
    void ModelConverter::LoadTextures()
    {
        std::vector<uint8_t> rgbaConvertBuffer{};
        for(int32_t i = 0; i < mGltfModel.textures.size(); i++)
        {
            const auto&        gltfTexture = mGltfModel.textures[i];
            const auto&        gltfImage   = mGltfModel.images[gltfTexture.source];
            mTextures.GetTextures().push_back(SampledTexture{.Image = std::make_unique<ManagedImage>(), .Sampler = nullptr});
            SampledTexture&     sampledTexture = mTextures.GetTextures().back();
            mIndexBindings.Textures[i] = sampledTexture.Image.get();

            const unsigned char* buffer     = nullptr;
            VkDeviceSize         bufferSize = 0;
            if(gltfImage.component == 3)
            {
                // Most devices don't support RGB only on Vulkan so convert if necessary
                // TODO: Check actual format support and transform only if required
                bufferSize = gltfImage.width * gltfImage.height * 4;
                rgbaConvertBuffer.resize(bufferSize);
                buffer                          = rgbaConvertBuffer.data();
                unsigned char*       rgba       = rgbaConvertBuffer.data();
                const unsigned char* rgb        = &gltfImage.image[0];
                int32_t              pixelCount = gltfImage.width * gltfImage.height;
                for(int32_t i = 0; i < pixelCount; ++i)
                {
                    for(int32_t j = 0; j < 3; ++j)
                    {
                        rgba[j] = rgb[j];
                    }
                    rgba += 4;
                    rgb += 3;
                }
            }
            else
            {
                buffer     = &gltfImage.image[0];
                bufferSize = gltfImage.image.size();
            }

            uint32_t   mipLevelCount = (uint32_t)(floorf(log2f(std::max(gltfImage.width, gltfImage.height))));
            VkExtent2D extent        = VkExtent2D{.width = (uint32_t)gltfImage.width, .height = (uint32_t)gltfImage.height};

            ManagedImage::CreateInfo imageCI;
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

            imageCI.Name = gltfTexture.name;
            if(!imageCI.Name.size())
            {
                imageCI.Name = gltfImage.name;
            }
            if(!imageCI.Name.size())
            {
                imageCI.Name = fmt::format("Texture #{}", i);
            }

            sampledTexture.Image->Create(mContext, imageCI);
            sampledTexture.Image->WriteDeviceLocalData(buffer, bufferSize, VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

            SingleTimeCommandBuffer cmdBuf;
            cmdBuf.Create(mContext, VkCommandBufferLevel::VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

            for(int32_t i = 0; i < mipLevelCount - 1; i++)
            {
                uint32_t sourceMipLevel = (uint32_t)i;
                uint32_t destMipLevel   = sourceMipLevel + 1;

                // Step #1 Transition
                VkImageSubresourceRange mipSubRange = {};
                mipSubRange.aspectMask              = VK_IMAGE_ASPECT_COLOR_BIT;
                mipSubRange.baseMipLevel            = destMipLevel;
                mipSubRange.levelCount              = 1;
                mipSubRange.layerCount              = 1;

                ManagedImage::LayoutTransitionInfo layoutTransition;
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
                VkOffset3D  dstArea{.x = (int32_t)extent.width >> i + 1, .y = (int32_t)extent.height >> i + 1, .z = 1};
                VkImageBlit blit{
                    .srcSubresource =
                        VkImageSubresourceLayers{.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT, .mipLevel = (uint32_t)i, .baseArrayLayer = 0, .layerCount = 1},
                    .dstSubresource = VkImageSubresourceLayers{
                        .aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT, .mipLevel = (uint32_t)i + 1, .baseArrayLayer = 0, .layerCount = 1}};
                blit.srcOffsets[1] = srcArea;
                blit.dstOffsets[1] = dstArea;
                vkCmdBlitImage(cmdBuf.GetCommandBuffer(), sampledTexture.Image->GetImage(), VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, sampledTexture.Image->GetImage(),
                               VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VkFilter::VK_FILTER_LINEAR);

                layoutTransition.OldImageLayout       = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                layoutTransition.NewImageLayout       = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                layoutTransition.BarrierSrcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                layoutTransition.BarrierDstAccessMask = 0;

                sampledTexture.Image->TransitionLayout(layoutTransition);
            }

            cmdBuf.Flush(true);

            VkSamplerCreateInfo samplerCI{.sType = VkStructureType::VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
            TranslateSampler(mGltfModel.samplers[gltfTexture.sampler], samplerCI);

            sampledTexture.Sampler = mTextures.GetOrCreateSampler(samplerCI);
        }
    }

    void ModelConverter::TranslateSampler(const tinygltf::Sampler& tinygltfSampler, VkSamplerCreateInfo& outsamplerCI)
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

}  // namespace hsk