#include "foray_envmap.hpp"
#include "../core/foray_commandbuffer.hpp"
#include "foray_imageloader.hpp"

namespace foray::util {

    struct LoadParams
    {
        core::HostSyncCommandBuffer& CmdBuffer;
        core::Context*           Context;
        const osi::Utf8Path&     Path;
        core::ManagedImage*      Image;
        VkImageUsageFlags        Usage;
        bool                     Final;
        VkImageLayout            AfterWrite;
        std::string_view         Name;
    };

    template <VkFormat format>
    void lLoadF(const LoadParams& params)
    {
        foray::util::ImageLoader<format> imageLoader;
        FORAY_ASSERTFMT(imageLoader.Init(params.Path) && imageLoader.Load(), "Failed to init / load envmap \"{}\"!", params.Path)

        VkExtent2D extent = imageLoader.GetInfo().Extent;

        foray::core::ManagedImage::CreateInfo ci(params.Usage, format, extent, params.Name);
        if(params.Final)
        {
            uint32_t mipCount                          = (uint32_t)(floorf(log2f(std::max(extent.width, extent.height))));
            ci.ImageCI.mipLevels                       = mipCount;
            ci.ImageViewCI.subresourceRange.levelCount = mipCount;
        }
        else
        {
            ci.CreateImageView = false;
        }
        imageLoader.InitManagedImage(params.Context, params.CmdBuffer, params.Image, ci, params.AfterWrite);
    }

    void lLoad(VkFormat format, const LoadParams& params)
    {
        switch(format)
        {
            case VkFormat::VK_FORMAT_R16G16B16A16_SFLOAT:
                lLoadF<VK_FORMAT_R16G16B16A16_SFLOAT>(params);
                break;
            case VkFormat::VK_FORMAT_R32G32B32A32_SFLOAT:
                lLoadF<VK_FORMAT_R32G32B32A32_SFLOAT>(params);
                break;
            case VkFormat::VK_FORMAT_R8G8B8A8_UNORM:
                lLoadF<VK_FORMAT_R8G8B8A8_UNORM>(params);
                break;
            case VkFormat::VK_FORMAT_R16G16B16_SFLOAT:
                lLoadF<VK_FORMAT_R16G16B16_SFLOAT>(params);
                break;
            case VkFormat::VK_FORMAT_R32G32B32_SFLOAT:
                lLoadF<VK_FORMAT_R32G32B32_SFLOAT>(params);
                break;
            case VkFormat::VK_FORMAT_R8G8B8_UNORM:
                lLoadF<VK_FORMAT_R8G8B8_UNORM>(params);
                break;
            case VkFormat::VK_FORMAT_R16G16_SFLOAT:
                lLoadF<VK_FORMAT_R16G16_SFLOAT>(params);
                break;
            case VkFormat::VK_FORMAT_R32G32_SFLOAT:
                lLoadF<VK_FORMAT_R32G32_SFLOAT>(params);
                break;
            case VkFormat::VK_FORMAT_R8G8_UNORM:
                lLoadF<VK_FORMAT_R8G8_UNORM>(params);
                break;
            case VkFormat::VK_FORMAT_R16_SFLOAT:
                lLoadF<VK_FORMAT_R16_SFLOAT>(params);
                break;
            case VkFormat::VK_FORMAT_R32_SFLOAT:
                lLoadF<VK_FORMAT_R32_SFLOAT>(params);
                break;
            case VkFormat::VK_FORMAT_R8_UNORM:
                lLoadF<VK_FORMAT_R8_UNORM>(params);
                break;
            default:
                break;
        }
    }


    void EnvironmentMap::Create(core::Context* context, const osi::Utf8Path& path, std::string_view name, VkFormat loadFormat, VkFormat storeFormat)
    {
        core::HostSyncCommandBuffer cmdBuffer;
        cmdBuffer.Create(context);

        VkExtent2D extent   = {};
        uint32_t   mipCount = 0;

        if(loadFormat == VkFormat::VK_FORMAT_UNDEFINED)
        {
            switch(storeFormat)
            {
                case VkFormat::VK_FORMAT_R32G32B32A32_SFLOAT:
                case VkFormat::VK_FORMAT_R16G16B16A16_SFLOAT:
                case VkFormat::VK_FORMAT_R8G8B8A8_UNORM:
                    loadFormat = VK_FORMAT_R32G32B32A32_SFLOAT;
                    break;
                case VkFormat::VK_FORMAT_R32G32B32_SFLOAT:
                case VkFormat::VK_FORMAT_R16G16B16_SFLOAT:
                case VkFormat::VK_FORMAT_R8G8B8_UNORM:
                    loadFormat = VK_FORMAT_R32G32B32_SFLOAT;
                    break;
                case VkFormat::VK_FORMAT_R32G32_SFLOAT:
                case VkFormat::VK_FORMAT_R16G16_SFLOAT:
                case VkFormat::VK_FORMAT_R8G8_UNORM:
                    loadFormat = VK_FORMAT_R32G32_SFLOAT;
                    break;
                case VkFormat::VK_FORMAT_R32_SFLOAT:
                case VkFormat::VK_FORMAT_R16_SFLOAT:
                case VkFormat::VK_FORMAT_R8_UNORM:
                    loadFormat = VK_FORMAT_R32_SFLOAT;
                    break;
                default:
                    Exception::Throw("Unable to autoset loadFormat!");
                    break;
            }
        }

        if(loadFormat != storeFormat)
        {
            core::ManagedImage temporaryImage;

            LoadParams params{.CmdBuffer  = cmdBuffer,
                              .Context    = context,
                              .Path       = path,
                              .Image      = &temporaryImage,
                              .Usage      = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                              .Final      = false,
                              .AfterWrite = VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                              .Name       = "Temporary Image"};

            lLoad(loadFormat, params);

            extent   = temporaryImage.GetExtent2D();
            mipCount = (uint32_t)(floorf(log2f(std::max(extent.width, extent.height))));
            core::ManagedImage::CreateInfo ci(VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, storeFormat, extent, name);
            ci.ImageCI.mipLevels                       = mipCount;
            ci.ImageViewCI.subresourceRange.levelCount = mipCount;

            mImage.Create(context, ci);

            cmdBuffer.Reset();
            cmdBuffer.Begin();

            VkImageMemoryBarrier2 dstBarrier{
                .sType               = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                .srcStageMask        = VK_PIPELINE_STAGE_2_NONE,
                .srcAccessMask       = VK_ACCESS_2_NONE,
                .dstStageMask        = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                .dstAccessMask       = VK_ACCESS_2_TRANSFER_WRITE_BIT,
                .oldLayout           = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED,
                .newLayout           = VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image               = mImage.GetImage(),
                .subresourceRange    = VkImageSubresourceRange{
                       .aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT, .baseMipLevel = 0, .levelCount = 1, .baseArrayLayer = 0, .layerCount = 1}};

            VkDependencyInfo depInfo{.sType = VkStructureType::VK_STRUCTURE_TYPE_DEPENDENCY_INFO, .imageMemoryBarrierCount = 1U, .pImageMemoryBarriers = &dstBarrier};

            vkCmdPipelineBarrier2(cmdBuffer, &depInfo);

            VkImageBlit2 blit{
                .sType          = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_BLIT_2,
                .srcSubresource = VkImageSubresourceLayers{.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT, .layerCount = 1},
                .srcOffsets     = {VkOffset3D{}, VkOffset3D{(int32_t)extent.width, (int32_t)extent.height, 1}},
                .dstSubresource = VkImageSubresourceLayers{.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT, .layerCount = 1},
                .dstOffsets     = {VkOffset3D{}, VkOffset3D{(int32_t)extent.width, (int32_t)extent.height, 1}},
            };

            VkBlitImageInfo2 blitInfo{.sType          = VkStructureType::VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2,
                                      .srcImage       = temporaryImage.GetImage(),
                                      .srcImageLayout = VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                      .dstImage       = mImage.GetImage(),
                                      .dstImageLayout = VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                      .regionCount    = 1U,
                                      .pRegions       = &blit,
                                      .filter         = VkFilter::VK_FILTER_NEAREST};

            vkCmdBlitImage2(cmdBuffer, &blitInfo);

            cmdBuffer.SubmitAndWait();
        }
        else
        {
            LoadParams params{.CmdBuffer  = cmdBuffer,
                              .Context    = context,
                              .Path       = path,
                              .Image      = &mImage,
                              .Usage      = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                              .Final      = true,
                              .AfterWrite = VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                              .Name       = name};

            lLoad(loadFormat, params);

            extent   = mImage.GetExtent2D();
            mipCount = mImage.GetCreateInfo().ImageCI.mipLevels;
        }

        {
            cmdBuffer.Reset();
            cmdBuffer.Begin();

            // Generate mip levels
            VkImage image = mImage.GetImage();

            std::vector<VkImageMemoryBarrier2> barriers(2);
            VkImageMemoryBarrier2&             sourceBarrier = barriers[0];
            VkImageMemoryBarrier2&             destBarrier   = barriers[1];

            sourceBarrier = VkImageMemoryBarrier2{.sType               = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                                                  .srcStageMask        = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                                                  .srcAccessMask       = VK_ACCESS_2_TRANSFER_WRITE_BIT,
                                                  .dstStageMask        = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                                                  .dstAccessMask       = VK_ACCESS_2_TRANSFER_READ_BIT,
                                                  .oldLayout           = VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                                  .newLayout           = VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                                  .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                                                  .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                                                  .image               = image,
                                                  .subresourceRange =
                                                      VkImageSubresourceRange{.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1}};
            destBarrier   = VkImageMemoryBarrier2{.sType               = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                                                  .srcStageMask        = VK_PIPELINE_STAGE_2_NONE,
                                                  .srcAccessMask       = VK_ACCESS_2_NONE,
                                                  .dstStageMask        = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                                                  .dstAccessMask       = VK_ACCESS_2_TRANSFER_WRITE_BIT,
                                                  .oldLayout           = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED,
                                                  .newLayout           = VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                                  .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                                                  .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                                                  .image               = image,
                                                  .subresourceRange =
                                                    VkImageSubresourceRange{.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1}};

            VkDependencyInfo depInfo{.sType = VkStructureType::VK_STRUCTURE_TYPE_DEPENDENCY_INFO, .imageMemoryBarrierCount = 2U, .pImageMemoryBarriers = barriers.data()};

            for(int32_t i = 0; i < (int32_t)mipCount - 1; i++)
            {
                uint32_t sourceMipLevel = (uint32_t)i;
                uint32_t destMipLevel   = sourceMipLevel + 1;

                sourceBarrier.subresourceRange.baseMipLevel = sourceMipLevel;
                destBarrier.subresourceRange.baseMipLevel   = destMipLevel;

                vkCmdPipelineBarrier2(cmdBuffer, &depInfo);

                VkOffset3D  srcArea{.x = (int32_t)extent.width >> i, .y = (int32_t)extent.height >> i, .z = 1};
                VkOffset3D  dstArea{.x = (int32_t)extent.width >> (i + 1), .y = (int32_t)extent.height >> (i + 1), .z = 1};
                VkImageBlit blit{
                    .srcSubresource =
                        VkImageSubresourceLayers{.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT, .mipLevel = (uint32_t)i, .baseArrayLayer = 0, .layerCount = 1},
                    .dstSubresource = VkImageSubresourceLayers{
                        .aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT, .mipLevel = (uint32_t)i + 1, .baseArrayLayer = 0, .layerCount = 1}};
                blit.srcOffsets[1] = srcArea;
                blit.dstOffsets[1] = dstArea;
                vkCmdBlitImage(cmdBuffer, image, VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image, VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit,
                               VkFilter::VK_FILTER_LINEAR);
            }

            {
                VkImageMemoryBarrier2 barriers[2] = {
                    VkImageMemoryBarrier2{.sType               = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                                          .srcStageMask        = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                                          .srcAccessMask       = VK_ACCESS_2_TRANSFER_READ_BIT | VK_ACCESS_2_TRANSFER_WRITE_BIT,
                                          .dstStageMask        = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
                                          .dstAccessMask       = VK_ACCESS_2_MEMORY_READ_BIT,
                                          .oldLayout           = VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                          .newLayout           = VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                          .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                                          .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                                          .image               = image,
                                          .subresourceRange =
                                              VkImageSubresourceRange{.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = mipCount - 1, .layerCount = 1}},
                    VkImageMemoryBarrier2{.sType               = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                                          .srcStageMask        = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                                          .srcAccessMask       = VK_ACCESS_2_TRANSFER_READ_BIT | VK_ACCESS_2_TRANSFER_WRITE_BIT,
                                          .dstStageMask        = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
                                          .dstAccessMask       = VK_ACCESS_2_MEMORY_READ_BIT,
                                          .oldLayout           = VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                          .newLayout           = VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                          .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                                          .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                                          .image               = image,
                                          .subresourceRange    = VkImageSubresourceRange{
                                                 .aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT, .baseMipLevel = mipCount - 1, .levelCount = 1U, .layerCount = 1}}};

                VkDependencyInfo depInfo{.sType = VkStructureType::VK_STRUCTURE_TYPE_DEPENDENCY_INFO, .imageMemoryBarrierCount = 2U, .pImageMemoryBarriers = barriers};

                vkCmdPipelineBarrier2(cmdBuffer, &depInfo);
            }

            cmdBuffer.SubmitAndWait();
        }

        {  // Init sampler
            VkSamplerCreateInfo samplerCi{.sType                   = VkStructureType::VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
                                          .magFilter               = VkFilter::VK_FILTER_LINEAR,
                                          .minFilter               = VkFilter::VK_FILTER_LINEAR,
                                          .addressModeU            = VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_REPEAT,
                                          .addressModeV            = VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_REPEAT,
                                          .addressModeW            = VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_REPEAT,
                                          .anisotropyEnable        = VK_FALSE,
                                          .compareEnable           = VK_FALSE,
                                          .minLod                  = 0,
                                          .maxLod                  = VK_LOD_CLAMP_NONE,
                                          .unnormalizedCoordinates = VK_FALSE};

            mSampler.Init(context->SamplerCol, samplerCi);
        }
    }

    void EnvironmentMap::Destroy()
    {
        mImage.Destroy();
        mSampler.Destroy();
    }
}  // namespace foray::util
