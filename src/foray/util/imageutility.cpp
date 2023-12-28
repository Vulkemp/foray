#include "imageutility.hpp"
#include "../core/commandbuffer.hpp"
#include "../core/managedbuffer.hpp"

namespace foray::util {
    ImageUploader::UploadChunk::UploadChunk(std::span<uint8_t> data) : mData(data) {}

    ImageUploader::UploadInfo::UploadInfo(const UploadChunk& chunk, bool generateMipChain) : mChunks({chunk}), mGenerateMipChain(generateMipChain) {}

    ImageUploader::UploadInfo::UploadInfo(std::span<UploadChunk> chunks, bool generateMipChain) : mChunks(chunks.begin(), chunks.end()), mGenerateMipChain(generateMipChain) {}

    ImageUploader::ImageUploader(core::Context* context, VkDeviceSize stagingBufferSize) : mContext(context)
    {
        core::ManagedBuffer::CreateInfo ci = core::ManagedBuffer::CreateForStaging(stagingBufferSize, "ImageUploader Staging");
        mStagingBuffer.New(mContext, ci);
        void* mapPoint = nullptr;
        mStagingBuffer->Map(mapPoint);
        mStagingBufferMapped = (uint8_t*)mapPoint;
        mCmdBuffer.New(mContext);
    }

    struct BarrierScope
    {
        BarrierScope() = default;
        inline BarrierScope(const ImageUploader::UploadChunk& chunk) : Aspect(chunk.GetAspectFlagBit()), MipLevel(chunk.GetMipLevel()), ArrayLayer(chunk.GetArrayLayer()) {}

        VkImageAspectFlagBits Aspect;
        uint32_t              MipLevel;
        uint32_t              ArrayLayer;

        inline bool operator==(const BarrierScope& other) const { return Aspect == other.Aspect && MipLevel == other.MipLevel && ArrayLayer == other.ArrayLayer; }
    };

    struct UploadGroup
    {
        core::IImage*                           Image         = nullptr;
        core::HostSyncCommandBuffer*            CmdBuffer     = nullptr;
        uint8_t*                                BufferMap     = nullptr;
        core::ManagedBuffer*                    Buffer        = nullptr;
        VkDeviceSize                            TotalDataSize = 0;
        std::vector<ImageUploader::UploadChunk> Chunks;
        vk::ImageLayout                           InitialLayout = vk::ImageLayout::VK_IMAGE_LAYOUT_UNDEFINED;
        vk::ImageLayout                           FinalLayout   = vk::ImageLayout::VK_IMAGE_LAYOUT_UNDEFINED;

        inline void Process(bool finalizeCmdBuffer = true)
        {
            CmdBuffer->Begin();

            std::vector<BarrierScope> barrierScopes;
            std::vector<VkDeviceSize> offsets;

            {
                // Barrier Scopes
                for(const ImageUploader::UploadChunk& chunk : Chunks)
                {
                    const BarrierScope chunkScope(chunk);
                    bool               found = false;
                    for(const BarrierScope& otherScope : barrierScopes)
                    {
                        if(chunkScope == otherScope)
                        {
                            found = true;
                            break;
                        }
                    }
                    if(!found)
                    {
                        barrierScopes.emplace_back(chunkScope);
                    }
                }
            }

            {  // Barriers
                VkImageMemoryBarrier2* barriers = FORAY_STACKALLOC(VkImageMemoryBarrier2, barrierScopes.size());
                for(uint32_t i = 0; i < (uint32_t)barrierScopes.size(); i++)
                {
                    const BarrierScope& scope = barrierScopes[i];
                    barriers[i]               = VkImageMemoryBarrier2{
                                      .sType               = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                                      .srcStageMask        = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
                                      .srcAccessMask       = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT,
                                      .dstStageMask        = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                                      .dstAccessMask       = VK_ACCESS_2_TRANSFER_WRITE_BIT,
                                      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                                      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                                      .oldLayout           = InitialLayout,
                                      .newLayout           = vk::ImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                      .image               = Image->GetImage(),
                                      .subresourceRange    = VkImageSubresourceRange{.aspectMask     = (vk::ImageAspectFlags)scope.Aspect,
                                                                                     .baseMipLevel   = scope.MipLevel,
                                                                                     .levelCount     = 1u,
                                                                                     .baseArrayLayer = scope.ArrayLayer,
                                                                                     .layerCount     = 1u},
                    };
                }
                VkDependencyInfo depInfo{.sType                   = VkStructureType::VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                                         .dependencyFlags         = VkDependencyFlagBits::VK_DEPENDENCY_BY_REGION_BIT,
                                         .imageMemoryBarrierCount = (uint32_t)barrierScopes.size(),
                                         .pImageMemoryBarriers    = barriers};
                vkCmdPipelineBarrier2(*CmdBuffer, &depInfo);
            }

            {  // Set up writes, simultaneously set up copy cmds
                const uint32_t chunkCount = (uint32_t)Chunks.size();
                VkBufferImageCopy2* copyRegions = FORAY_STACKALLOC(VkBufferImageCopy2, chunkCount);

                std::size_t offset = 0;
                for(uint32_t region = 0; region < chunkCount; region++)
                {
                    const ImageUploader::UploadChunk& upload = Chunks[region];
                    const std::span<uint8_t>&         data   = upload.GetData();
                    std::memcpy(BufferMap + offset, data.data(), data.size());

                    copyRegions[region] = VkBufferImageCopy2{.sType            = VkStructureType::VK_STRUCTURE_TYPE_BUFFER_IMAGE_COPY_2,
                                                             .bufferOffset     = offset,
                                                             .imageSubresource = VkImageSubresourceLayers{.aspectMask     = (vk::ImageAspectFlags)upload.GetAspectFlagBit(),
                                                                                                          .mipLevel       = upload.GetMipLevel(),
                                                                                                          .baseArrayLayer = upload.GetArrayLayer(),
                                                                                                          .layerCount     = 1u},
                                                             .imageOffset      = upload.GetOffset(),
                                                             .imageExtent      = upload.GetExtent()};

                    offset += data.size();
                }

                VkCopyBufferToImageInfo2 bufferToImageInfo{
                    .sType          = VkStructureType::VK_STRUCTURE_TYPE_COPY_BUFFER_TO_IMAGE_INFO_2,
                    .srcBuffer      = Buffer->GetBuffer(),
                    .dstImage       = Image->GetImage(),
                    .dstImageLayout = vk::ImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    .regionCount    = chunkCount,
                    .pRegions       = copyRegions,
                };
                vkCmdCopyBufferToImage2(*CmdBuffer, &bufferToImageInfo);
            }

            {  // Translate touched resources to final layout
                VkImageMemoryBarrier2* barriers = FORAY_STACKALLOC(VkImageMemoryBarrier2, barrierScopes.size());
                for(uint32_t i = 0; i < (uint32_t)barrierScopes.size(); i++)
                {
                    const BarrierScope& scope = barrierScopes[i];
                    barriers[i]               = VkImageMemoryBarrier2{
                                      .sType               = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                                      .srcStageMask        = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                                      .srcAccessMask       = VK_ACCESS_2_TRANSFER_WRITE_BIT,
                                      .dstStageMask        = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
                                      .dstAccessMask       = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT,
                                      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                                      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                                      .oldLayout           = vk::ImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                      .newLayout           = FinalLayout,
                                      .image               = Image->GetImage(),
                                      .subresourceRange    = VkImageSubresourceRange{.aspectMask     = (vk::ImageAspectFlags)scope.Aspect,
                                                                                     .baseMipLevel   = scope.MipLevel,
                                                                                     .levelCount     = 1u,
                                                                                     .baseArrayLayer = scope.ArrayLayer,
                                                                                     .layerCount     = 1u},
                    };
                }
                VkDependencyInfo depInfo{.sType                   = VkStructureType::VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                                         .dependencyFlags         = VkDependencyFlagBits::VK_DEPENDENCY_BY_REGION_BIT,
                                         .imageMemoryBarrierCount = (uint32_t)barrierScopes.size(),
                                         .pImageMemoryBarriers    = barriers};
                vkCmdPipelineBarrier2(*CmdBuffer, &depInfo);
            }

            if(finalizeCmdBuffer)
            {
                CmdBuffer->End();
                CmdBuffer->SubmitAndWait();
            }
        }
    };

    void ImageUploader::UploadSynchronized(core::IImage* image, const UploadInfo& uploadInfo)
    {
        const uint32_t chunkCount = (uint32_t)uploadInfo.GetChunks().size();

        std::vector<UploadGroup> uploadGroups;

        for(const UploadChunk& chunk : uploadInfo.GetChunks())
        {
            const VkDeviceSize chunkSize = chunk.GetData().size();
            if(chunkSize > mStagingBuffer->GetSize())
            {
                core::ManagedBuffer::CreateInfo ci = core::ManagedBuffer::CreateForStaging(chunkSize, "ImageUploader Staging");
                mStagingBuffer.New(mContext, ci);
                // TODO Joseph: It's probably possible to break large chunks into smaller chunks instead!
            }
            bool foundExistingGroup = false;
            for(UploadGroup& group : uploadGroups)
            {
                const VkDeviceSize groupBudget = mStagingBuffer->GetSize() - group.TotalDataSize;
                if(chunkSize <= groupBudget)
                {
                    group.Chunks.emplace_back(chunk);
                    group.TotalDataSize += chunkSize;
                    foundExistingGroup = true;
                    break;
                }
            }
            if(!foundExistingGroup)
            {
                uploadGroups.emplace_back(
                    UploadGroup{.Image         = image,
                                .CmdBuffer     = mCmdBuffer.Get(),
                                .BufferMap     = mStagingBufferMapped,
                                .Buffer        = mStagingBuffer.Get(),
                                .TotalDataSize = chunkSize,
                                .Chunks        = {chunk},
                                .InitialLayout = uploadInfo.GetInitialLayout(),
                                .FinalLayout   = uploadInfo.GetGenerateMipChain() ? vk::ImageLayout::VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL : uploadInfo.GetFinalLayout()});
            }
        }

        for(uint32_t i = 0; i < (uint32_t)uploadGroups.size(); i++)
        {
            UploadGroup& group = uploadGroups[i];
            group.Process(i + 1 < (uint32_t)uploadGroups.size());  // Resynchronize all but the last group
        }

        if(uploadInfo.GetGenerateMipChain())
        {
            ImageUtility::CmdGenerateMipChain(mCmdBuffer.GetRef(), image, vk::ImageLayout::VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, uploadInfo.GetFinalLayout());
        }

        // We defer the last group until here to pack the mip chain generation into the same command submission
        mCmdBuffer->End();
        mCmdBuffer->SubmitAndWait();
    }

    void ImageUploader::UploadSynchronized(core::IImage* image, const UploadChunk& uploadChunk, bool generateMipChain, vk::ImageLayout initialLayout, vk::ImageLayout finalLayout)
    {
        UploadInfo uploadInfo(uploadChunk);
        uploadInfo.SetGenerateMipChain(generateMipChain).SetInitialLayout(initialLayout).SetFinalLayout(finalLayout);
        UploadSynchronized(image, uploadInfo);
    }

    void ImageUtility::GenerateMipChainSynchronized(core::Context* context, core::IImage* image, vk::ImageLayout mip0SrcLayout, vk::ImageLayout allMipsDstLayout)
    {
        core::HostSyncCommandBuffer cmdBuffer(context);
        cmdBuffer.Begin();
        CmdGenerateMipChain(cmdBuffer, image, mip0SrcLayout, std::span<vk::ImageLayout>(&allMipsDstLayout, 1));
        cmdBuffer.SubmitAndWait();
    }
    void ImageUtility::GenerateMipChainSynchronized(core::Context* context, core::IImage* image, vk::ImageLayout mip0SrcLayout, std::span<vk::ImageLayout> mipsDstLayouts)
    {
        core::HostSyncCommandBuffer cmdBuffer(context);
        cmdBuffer.Begin();
        CmdGenerateMipChain(cmdBuffer, image, mip0SrcLayout, mipsDstLayouts);
        cmdBuffer.SubmitAndWait();
    }
    void ImageUtility::CmdGenerateMipChain(VkCommandBuffer cmdBuffer, core::IImage* image, vk::ImageLayout mip0SrcLayout, vk::ImageLayout allMipsDstLayout)
    {
        CmdGenerateMipChain(cmdBuffer, image, mip0SrcLayout, std::span<vk::ImageLayout>(&allMipsDstLayout, 1));
    }
    void ImageUtility::CmdGenerateMipChain(VkCommandBuffer cmdBuffer, core::IImage* image, vk::ImageLayout mip0SrcLayout, std::span<vk::ImageLayout> mipsDstLayouts)
    {
        // Establish some base values

        const uint32_t              mipCount   = image->GetMipLevelCount();  // Image total mip level count
        constexpr vk::ImageLayout     layoutSrc  = vk::ImageLayout::VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        constexpr vk::ImageLayout     layoutDst  = vk::ImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        const vk::ImageAspectFlags    aspectMask = GetFormatAspectFlags(image->GetFormat());  // Aspect flags describing all aspects of the image
        const VkImageMemoryBarrier2 barrierTemplate  // A template for all image mem barriers processing this image during mip generation
            {
                .sType               = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image               = image->GetImage(),
                .subresourceRange    = VkImageSubresourceRange{.aspectMask = aspectMask, .baseArrayLayer = 0u, .layerCount = VK_REMAINING_ARRAY_LAYERS},
            };

        vk::ImageLayout* mipLayouts = (vk::ImageLayout*)alloca(sizeof(vk::ImageLayout) * mipCount);  // A stack-allocated cache of ImageLayouts, per mip level

        {  // Check some things
            Assert(mipCount >= 2u, "Mip count must be atleast 2");
            Assert(mipsDstLayouts.size() >= 1u, "Must define atleast 1 mip destination layout");
        }


        {  // Transfer all mip levels to the starting layout
            VkImageMemoryBarrier2 mip0Barrier         = barrierTemplate;  // barrier old -> src for mip level 0
            mip0Barrier.srcStageMask                  = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
            mip0Barrier.srcAccessMask                 = VK_ACCESS_2_MEMORY_READ_BIT;
            mip0Barrier.dstStageMask                  = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
            mip0Barrier.dstAccessMask                 = VK_ACCESS_2_TRANSFER_READ_BIT;
            mip0Barrier.oldLayout                     = mip0SrcLayout;
            mip0Barrier.newLayout                     = layoutSrc;
            mip0Barrier.subresourceRange.baseMipLevel = 0u;
            mip0Barrier.subresourceRange.levelCount   = 1u;

            VkImageMemoryBarrier2 mip1ppBarrier         = barrierTemplate;  // prepare the remaining mip levels for receiving transfers
            mip1ppBarrier.srcStageMask                  = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
            mip1ppBarrier.srcAccessMask                 = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT;
            mip1ppBarrier.dstStageMask                  = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
            mip1ppBarrier.dstAccessMask                 = VK_ACCESS_2_TRANSFER_WRITE_BIT;
            mip1ppBarrier.oldLayout                     = vk::ImageLayout::VK_IMAGE_LAYOUT_UNDEFINED;
            mip1ppBarrier.newLayout                     = layoutDst;
            mip1ppBarrier.subresourceRange.baseMipLevel = 1u;
            mip1ppBarrier.subresourceRange.levelCount   = mipCount - 1u;

            VkImageMemoryBarrier2 barriers[2] = {mip0Barrier, mip1ppBarrier};

            VkDependencyInfo depInfo{
                .sType                   = VkStructureType::VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                .dependencyFlags         = VkDependencyFlagBits::VK_DEPENDENCY_BY_REGION_BIT,
                .imageMemoryBarrierCount = 2u,
                .pImageMemoryBarriers    = barriers,
            };

            vkCmdPipelineBarrier2(cmdBuffer, &depInfo);

            mipLayouts[0] = layoutSrc;
            for(uint32_t level = 1; level < mipCount; level++)
            {
                mipLayouts[level] = layoutDst;
            }
        }

        {  // Iteratively process levels

            // Cached values for blitting

            VkImageBlit2 imageBlit  // Imageblit structure defines resources and coordinates to blit
                {.sType          = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_BLIT_2,
                 .srcSubresource = VkImageSubresourceLayers{.aspectMask = aspectMask, .baseArrayLayer = 0u, .layerCount = VK_REMAINING_ARRAY_LAYERS},
                 .dstSubresource = VkImageSubresourceLayers{.aspectMask = aspectMask, .baseArrayLayer = 0u, .layerCount = VK_REMAINING_ARRAY_LAYERS}};

            const VkBlitImageInfo2 blitImageInfo  // blit image info determines the actual blit command
                {
                    .sType          = VkStructureType::VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2,
                    .srcImage       = image->GetImage(),
                    .srcImageLayout = layoutSrc,
                    .dstImage       = image->GetImage(),
                    .dstImageLayout = layoutDst,
                    .regionCount    = 1u,
                    .pRegions       = &imageBlit,
                    .filter         = VkFilter::VK_FILTER_LINEAR,
                };

            const vk::Extent3D imageExtent  = image->GetExtent();
            VkOffset3D*      levelOffsets = (VkOffset3D*)alloca(sizeof(VkOffset3D) * mipCount);  // Stack-allocated array of texel coordinates for the far corner of each mip level
            for(int32_t level = 0u; level < mipCount; level++)
            {
                levelOffsets[level].x = std::max<int32_t>((int32_t)(imageExtent.width >> level) - 1, 1);
                levelOffsets[level].y = std::max<int32_t>((int32_t)(imageExtent.height >> level) - 1, 1);
                levelOffsets[level].z = std::max<int32_t>((int32_t)(imageExtent.depth >> level) - 1, 1);
            }

            // Cached values for memory barriers

            VkImageMemoryBarrier2 toSrcTemplate       = barrierTemplate;  // Template for transferring a miplevel to dst
            toSrcTemplate.srcStageMask                = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
            toSrcTemplate.srcAccessMask               = VK_ACCESS_2_TRANSFER_WRITE_BIT;
            toSrcTemplate.dstStageMask                = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
            toSrcTemplate.dstAccessMask               = VK_ACCESS_2_TRANSFER_READ_BIT;
            toSrcTemplate.newLayout                   = layoutSrc;
            toSrcTemplate.subresourceRange.levelCount = 1u;

            VkImageMemoryBarrier2 finalizeTemplate       = barrierTemplate;  // Template for transferring a miplevel to the final layout
            finalizeTemplate.srcStageMask                = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
            finalizeTemplate.srcAccessMask               = VK_ACCESS_2_TRANSFER_READ_BIT;
            finalizeTemplate.dstStageMask                = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
            finalizeTemplate.dstAccessMask               = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT;
            finalizeTemplate.subresourceRange.levelCount = 1u;

            VkImageMemoryBarrier2 barriers[2];  // barrier array
            VkDependencyInfo      depInfo  // dep info (only required to set imageMemoryBarrierCount)
                {
                    .sType                = VkStructureType::VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                    .dependencyFlags      = VkDependencyFlagBits::VK_DEPENDENCY_BY_REGION_BIT,
                    .pImageMemoryBarriers = barriers,
                };


            for(int32_t level = 0u; level < mipCount + 1u; level++)
            {
                const int32_t srcLevel      = level;  // The blit src level
                const int32_t dstLevel      = srcLevel + 1;  // The blit dst level
                const int32_t finalizeLevel = level - 1;  // The level to finalize to the final layout

                if(srcLevel > 0 && dstLevel < mipCount)
                {  // Blit
                    imageBlit.srcSubresource.mipLevel = srcLevel;
                    imageBlit.srcOffsets[1]           = levelOffsets[srcLevel];
                    imageBlit.dstSubresource.mipLevel = dstLevel;
                    imageBlit.dstOffsets[1]           = levelOffsets[dstLevel];

                    vkCmdBlitImage2(cmdBuffer, &blitImageInfo);
                }

                {  // Memory barriers

                    uint32_t usedBarriers = 0u;
                    if(dstLevel >= 1 && dstLevel < mipCount - 1u)
                    {  // After the blit, transfer the previous dst miplevel to now be a src miplevel for the next level down
                        VkImageMemoryBarrier2 toSrc         = toSrcTemplate;
                        toSrc.oldLayout                     = mipLayouts[dstLevel];
                        toSrc.subresourceRange.baseMipLevel = (uint32_t)dstLevel;

                        barriers[usedBarriers++] = toSrc;
                        mipLayouts[dstLevel]     = toSrc.newLayout;
                    }
                    if(finalizeLevel >= 0 && finalizeLevel < mipCount)
                    {  // After the blit, transfer the src miplevel to the final layout (it won't be touched again)
                        VkImageMemoryBarrier2 finalize         = finalizeTemplate;
                        finalize.oldLayout                     = mipLayouts[finalizeLevel];
                        finalize.newLayout                     = mipsDstLayouts[finalizeLevel % (int32_t)mipsDstLayouts.size()];
                        finalize.subresourceRange.baseMipLevel = (uint32_t)finalizeLevel;


                        barriers[usedBarriers++]  = finalize;
                        mipLayouts[finalizeLevel] = finalize.newLayout;
                    }

                    if(usedBarriers > 0u)
                    {
                        depInfo.imageMemoryBarrierCount = usedBarriers;

                        vkCmdPipelineBarrier2(cmdBuffer, &depInfo);
                    }
                }
            }
        }
    }

    constexpr vk::ImageAspectFlags DEPTH        = VkImageAspectFlagBits::VK_IMAGE_ASPECT_DEPTH_BIT;
    constexpr vk::ImageAspectFlags STENCIL      = VkImageAspectFlagBits::VK_IMAGE_ASPECT_STENCIL_BIT;
    constexpr vk::ImageAspectFlags DEPTHSTENCIL = VkImageAspectFlagBits::VK_IMAGE_ASPECT_DEPTH_BIT | VkImageAspectFlagBits::VK_IMAGE_ASPECT_STENCIL_BIT;

    const std::unordered_map<vk::Format, vk::ImageAspectFlags> mFormatToAspectFlags = {{vk::Format::VK_FORMAT_D16_UNORM, DEPTH},
                                                                                   {vk::Format::VK_FORMAT_X8_D24_UNORM_PACK32, DEPTH},
                                                                                   {vk::Format::VK_FORMAT_D32_SFLOAT, DEPTH},
                                                                                   {vk::Format::VK_FORMAT_S8_UINT, STENCIL},
                                                                                   {vk::Format::VK_FORMAT_D16_UNORM_S8_UINT, DEPTHSTENCIL},
                                                                                   {vk::Format::VK_FORMAT_D24_UNORM_S8_UINT, DEPTHSTENCIL},
                                                                                   {vk::Format::VK_FORMAT_D32_SFLOAT_S8_UINT, DEPTHSTENCIL}};

    vk::ImageAspectFlags ImageUtility::GetFormatAspectFlags(vk::Format format)
    {
        const auto iter = mFormatToAspectFlags.find(format);
        if(iter != mFormatToAspectFlags.cend())
        {
            return iter->second;
        }
        else
        {
            return VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
        }
    }
}  // namespace foray::util