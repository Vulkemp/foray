#pragma once
#include "../core/commandbuffer.hpp"
#include "../core/image.hpp"
#include "../core/managedbuffer.hpp"
#include <span>

namespace foray::util {

    class ImageUploader : public NoMoveDefaults
    {
      public:
        class UploadChunk
        {
          public:
            UploadChunk() = default;
            UploadChunk(std::span<uint8_t> data);

            FORAY_PROPERTY_V(Data)
            FORAY_PROPERTY_V(MipLevel)
            FORAY_PROPERTY_V(ArrayLayer)
            FORAY_PROPERTY_V(AspectFlagBit)
            FORAY_PROPERTY_V(Offset)
            FORAY_PROPERTY_V(Extent)
          protected:
            std::span<uint8_t>    mData          = {};
            uint32_t              mMipLevel      = 0;
            uint32_t              mArrayLayer    = 0;
            VkImageAspectFlagBits mAspectFlagBit = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
            VkOffset3D            mOffset        = {};
            vk::Extent3D            mExtent        = {};
        };

        class UploadInfo
        {
          public:
            UploadInfo() = default;
            UploadInfo(const UploadChunk& chunk, bool generateMipChain = false);
            UploadInfo(std::span<UploadChunk> chunks, bool generateMipChain = false);

            FORAY_PROPERTY_R(Chunks)
            FORAY_PROPERTY_V(GenerateMipChain)
            FORAY_PROPERTY_V(InitialLayout)
            FORAY_PROPERTY_V(FinalLayout)
          protected:
            std::vector<UploadChunk> mChunks;
            bool                     mGenerateMipChain = false;
            vk::ImageLayout            mInitialLayout    = vk::ImageLayout::VK_IMAGE_LAYOUT_UNDEFINED;
            vk::ImageLayout            mFinalLayout      = vk::ImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        };

        ImageUploader(core::Context* context, VkDeviceSize stagingBufferSize = 1024 * 1024 * 512);
        virtual ~ImageUploader() = default;

        virtual void UploadSynchronized(core::IImage* image, const UploadInfo& uploadInfo);
        virtual void UploadSynchronized(core::IImage*      image,
                                        const UploadChunk& uploadChunk,
                                        bool               generateMipChain = false,
                                        vk::ImageLayout      initialLayout    = vk::ImageLayout::VK_IMAGE_LAYOUT_UNDEFINED,
                                        vk::ImageLayout      finalLayout      = vk::ImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

      protected:
        core::Context*                     mContext = nullptr;
        Local<core::ManagedBuffer>         mStagingBuffer;
        uint8_t*                           mStagingBufferMapped;
        Local<core::HostSyncCommandBuffer> mCmdBuffer;
    };

    class ImageUtility
    {
      public:
        static void               GenerateMipChainSynchronized(core::Context* context, core::IImage* image, vk::ImageLayout mip0SrcLayout, vk::ImageLayout allMipsDstLayout);
        static void               GenerateMipChainSynchronized(core::Context* context, core::IImage* image, vk::ImageLayout mip0SrcLayout, std::span<vk::ImageLayout> mipsDstLayouts);
        static void               CmdGenerateMipChain(VkCommandBuffer cmdBuffer, core::IImage* image, vk::ImageLayout mip0SrcLayout, vk::ImageLayout allMipsDstLayout);
        static void               CmdGenerateMipChain(VkCommandBuffer cmdBuffer, core::IImage* image, vk::ImageLayout mip0SrcLayout, std::span<vk::ImageLayout> mipsDstLayouts);
        static vk::ImageAspectFlags GetFormatAspectFlags(vk::Format format);
    };
}  // namespace foray::util
