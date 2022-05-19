#include "hsk_texture.hpp"
#include "../hsk_vkHelpers.hpp"
#include "../memory/hsk_vmaHelpers.hpp"
#include "hsk_scene.hpp"

namespace hsk {

    void TextureSampler::InitFromTinyGltfSampler(const tinygltf::Sampler& sampler)
    {
        MinFilter    = sTranslateToVkFilterMode(sampler.minFilter);
        MagFilter    = sTranslateToVkFilterMode(sampler.magFilter);
        AddressModeU = sTranslateToVkWrapMode(sampler.wrapS);
        AddressModeV = sTranslateToVkWrapMode(sampler.wrapT);
        AddressModeW = AddressModeV;
    }

    VkSamplerAddressMode TextureSampler::sTranslateToVkWrapMode(int32_t wrapMode)
    {
        switch(wrapMode)
        {
            case 10497:
                return VK_SAMPLER_ADDRESS_MODE_REPEAT;
            case 33071:
                return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            case 33648:
                return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
            default:
                HSK_THROWFMT("TextureSampler::getVkWrapMode: Unable to convert wrapMode {}", wrapMode);
        }
    }

    VkFilter TextureSampler::sTranslateToVkFilterMode(int32_t filterMode)
    {
        switch(filterMode)
        {
            case 9728:
                return VK_FILTER_NEAREST;
            case 9729:
                return VK_FILTER_LINEAR;
            case 9984:
                return VK_FILTER_NEAREST;
            case 9985:
                return VK_FILTER_NEAREST;
            case 9986:
                return VK_FILTER_LINEAR;
            case 9987:
                return VK_FILTER_LINEAR;
            default:
                HSK_THROWFMT("TextureSampler::getVkFilterMode: Unable to convert filterMode {}", filterMode)
        }
    }


    Texture::Texture() {}
    Texture::Texture(hsk::Scene* scene) : SceneComponent(scene) {}

    void Texture::InitFromTinyGltfImage(tinygltf::Image& gltfimage, TextureSampler textureSampler)
    {

        unsigned char* buffer       = nullptr;
        VkDeviceSize   bufferSize   = 0;
        bool           deleteBuffer = false;
        if(gltfimage.component == 3)
        {
            // Most devices don't support RGB only on Vulkan so convert if necessary
            // TODO: Check actual format support and transform only if required
            bufferSize          = gltfimage.width * gltfimage.height * 4;
            buffer              = new unsigned char[bufferSize];
            unsigned char* rgba = buffer;
            unsigned char* rgb  = &gltfimage.image[0];
            for(int32_t i = 0; i < gltfimage.width * gltfimage.height; ++i)
            {
                for(int32_t j = 0; j < 3; ++j)
                {
                    rgba[j] = rgb[j];
                }
                rgba += 4;
                rgb += 3;
            }
            deleteBuffer = true;
        }
        else
        {
            buffer     = &gltfimage.image[0];
            bufferSize = gltfimage.image.size();
        }

        VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;

        VkFormatProperties formatProperties;

        mExtent.width  = gltfimage.width;
        mExtent.height = gltfimage.height;
        mMipLevels     = static_cast<uint32_t>(floor(log2(std::max(mExtent.width, mExtent.height))) + 1.0);

        vkGetPhysicalDeviceFormatProperties(Context()->PhysicalDevice, format, &formatProperties);
        assert(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT);
        assert(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT);

        VkMemoryAllocateInfo memAllocInfo{};
        memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        VkMemoryRequirements memReqs{};

        VmaAllocationCreateInfo allocInfo = {};
        allocInfo.usage                   = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
        allocInfo.flags                   = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

        VmaAllocation stagingAllocation;
        VkBuffer      stagingBuffer;

        createBuffer(Context()->Allocator, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, allocInfo, &stagingAllocation, bufferSize, &stagingBuffer, buffer);

        VkImageCreateInfo imageCreateInfo{};
        imageCreateInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageCreateInfo.imageType     = VK_IMAGE_TYPE_2D;
        imageCreateInfo.format        = format;
        imageCreateInfo.mipLevels     = mMipLevels;
        imageCreateInfo.arrayLayers   = 1;
        imageCreateInfo.samples       = VK_SAMPLE_COUNT_1_BIT;
        imageCreateInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
        imageCreateInfo.usage         = VK_IMAGE_USAGE_SAMPLED_BIT;
        imageCreateInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
        imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageCreateInfo.extent        = mExtent;
        imageCreateInfo.usage         = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

        VmaAllocationCreateInfo imageAllocInfo = {};
        allocInfo.usage                        = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
        // allocInfo.flags                   = VMA_ALLOCATION_CREATE;


        AssertVkResult(vmaCreateImage(Context()->Allocator, &imageCreateInfo, &imageAllocInfo, &mImage, &mAllocation, nullptr));

        VkCommandBuffer copyCmd = createCommandBuffer(Context()->Device, Context()->TransferCommandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

        VkImageSubresourceRange subresourceRange = {};
        subresourceRange.aspectMask              = VK_IMAGE_ASPECT_COLOR_BIT;
        subresourceRange.levelCount              = 1;
        subresourceRange.layerCount              = 1;

        {
            VkImageMemoryBarrier imageMemoryBarrier{};
            imageMemoryBarrier.sType            = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            imageMemoryBarrier.oldLayout        = VK_IMAGE_LAYOUT_UNDEFINED;
            imageMemoryBarrier.newLayout        = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            imageMemoryBarrier.srcAccessMask    = 0;
            imageMemoryBarrier.dstAccessMask    = VK_ACCESS_TRANSFER_WRITE_BIT;
            imageMemoryBarrier.image            = mImage;
            imageMemoryBarrier.subresourceRange = subresourceRange;
            vkCmdPipelineBarrier(copyCmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
        }

        VkBufferImageCopy bufferCopyRegion               = {};
        bufferCopyRegion.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        bufferCopyRegion.imageSubresource.mipLevel       = 0;
        bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
        bufferCopyRegion.imageSubresource.layerCount     = 1;
        bufferCopyRegion.imageExtent                     = mExtent;
        vkCmdCopyBufferToImage(copyCmd, stagingBuffer, mImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &bufferCopyRegion);

        {
            VkImageMemoryBarrier imageMemoryBarrier{};
            imageMemoryBarrier.sType            = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            imageMemoryBarrier.oldLayout        = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            imageMemoryBarrier.newLayout        = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            imageMemoryBarrier.srcAccessMask    = VK_ACCESS_TRANSFER_WRITE_BIT;
            imageMemoryBarrier.dstAccessMask    = VK_ACCESS_TRANSFER_READ_BIT;
            imageMemoryBarrier.image            = mImage;
            imageMemoryBarrier.subresourceRange = subresourceRange;
            vkCmdPipelineBarrier(copyCmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
        }

        flushCommandBuffer(Context()->Device, Context()->TransferCommandPool, copyCmd, Context()->TransferQueue, true);
        vmaDestroyBuffer(Context()->Allocator, stagingBuffer, stagingAllocation);

        // Generate the mip chain (glTF uses jpg and png, so we need to create this manually)
        VkCommandBuffer blitCmd = createCommandBuffer(Context()->Device, Context()->TransferCommandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
        for(uint32_t i = 1; i < mMipLevels; i++)
        {
            VkImageBlit imageBlit{};

            imageBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            imageBlit.srcSubresource.layerCount = 1;
            imageBlit.srcSubresource.mipLevel   = i - 1;
            imageBlit.srcOffsets[1].x           = int32_t(mExtent.width >> (i - 1));
            imageBlit.srcOffsets[1].y           = int32_t(mExtent.height >> (i - 1));
            imageBlit.srcOffsets[1].z           = 1;

            imageBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            imageBlit.dstSubresource.layerCount = 1;
            imageBlit.dstSubresource.mipLevel   = i;
            imageBlit.dstOffsets[1].x           = int32_t(mExtent.width >> i);
            imageBlit.dstOffsets[1].y           = int32_t(mExtent.height >> i);
            imageBlit.dstOffsets[1].z           = 1;

            VkImageSubresourceRange mipSubRange = {};
            mipSubRange.aspectMask              = VK_IMAGE_ASPECT_COLOR_BIT;
            mipSubRange.baseMipLevel            = i;
            mipSubRange.levelCount              = 1;
            mipSubRange.layerCount              = 1;

            {
                VkImageMemoryBarrier imageMemoryBarrier{};
                imageMemoryBarrier.sType            = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                imageMemoryBarrier.oldLayout        = VK_IMAGE_LAYOUT_UNDEFINED;
                imageMemoryBarrier.newLayout        = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                imageMemoryBarrier.srcAccessMask    = 0;
                imageMemoryBarrier.dstAccessMask    = VK_ACCESS_TRANSFER_WRITE_BIT;
                imageMemoryBarrier.image            = mImage;
                imageMemoryBarrier.subresourceRange = mipSubRange;
                vkCmdPipelineBarrier(blitCmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
            }

            vkCmdBlitImage(blitCmd, mImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, mImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageBlit, VK_FILTER_LINEAR);

            {
                VkImageMemoryBarrier imageMemoryBarrier{};
                imageMemoryBarrier.sType            = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                imageMemoryBarrier.oldLayout        = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                imageMemoryBarrier.newLayout        = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                imageMemoryBarrier.srcAccessMask    = VK_ACCESS_TRANSFER_WRITE_BIT;
                imageMemoryBarrier.dstAccessMask    = VK_ACCESS_TRANSFER_READ_BIT;
                imageMemoryBarrier.image            = mImage;
                imageMemoryBarrier.subresourceRange = mipSubRange;
                vkCmdPipelineBarrier(blitCmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
            }
        }

        subresourceRange.levelCount = mMipLevels;
        mImageLayout                = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        {
            VkImageMemoryBarrier imageMemoryBarrier{};
            imageMemoryBarrier.sType            = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            imageMemoryBarrier.oldLayout        = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            imageMemoryBarrier.newLayout        = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageMemoryBarrier.srcAccessMask    = VK_ACCESS_TRANSFER_WRITE_BIT;
            imageMemoryBarrier.dstAccessMask    = VK_ACCESS_TRANSFER_READ_BIT;
            imageMemoryBarrier.image            = mImage;
            imageMemoryBarrier.subresourceRange = subresourceRange;
            vkCmdPipelineBarrier(blitCmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
        }

        flushCommandBuffer(Context()->Device, Context()->TransferCommandPool, blitCmd, Context()->TransferQueue, true);

        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType            = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter        = textureSampler.MagFilter;
        samplerInfo.minFilter        = textureSampler.MinFilter;
        samplerInfo.mipmapMode       = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.addressModeU     = textureSampler.AddressModeU;
        samplerInfo.addressModeV     = textureSampler.AddressModeV;
        samplerInfo.addressModeW     = textureSampler.AddressModeW;
        samplerInfo.compareOp        = VK_COMPARE_OP_NEVER;
        samplerInfo.borderColor      = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
        samplerInfo.maxAnisotropy    = 1.0;
        samplerInfo.anisotropyEnable = VK_FALSE;
        samplerInfo.maxLod           = (float)mMipLevels;
        // samplerInfo.maxAnisotropy    = 8.0f;
        // samplerInfo.anisotropyEnable = VK_TRUE;
        AssertVkResult(vkCreateSampler(Context()->Device, &samplerInfo, nullptr, &mSampler));

        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType                       = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image                       = mImage;
        viewInfo.viewType                    = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format                      = format;
        viewInfo.components                  = {VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A};
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.layerCount = 1;
        viewInfo.subresourceRange.levelCount = mMipLevels;
        AssertVkResult(vkCreateImageView(Context()->Device, &viewInfo, nullptr, &mImageView));

        mDescriptor.sampler     = mSampler;
        mDescriptor.imageView   = mImageView;
        mDescriptor.imageLayout = mImageLayout;

        if(deleteBuffer)
            delete[] buffer;
    }

    void Texture::UpdateDescriptor()
    {
        mDescriptor.sampler     = mSampler;
        mDescriptor.imageView   = mImageView;
        mDescriptor.imageLayout = mImageLayout;
    }

    void Texture::Cleanup()
    {
        vkDestroyImageView(Context()->Device, mImageView, nullptr);
        vmaDestroyImage(Context()->Allocator, mImage, mAllocation);
        vkDestroySampler(Context()->Device, mSampler, nullptr);
        mImage      = nullptr;
        mImageView  = nullptr;
        mAllocation = nullptr;
        mSampler    = nullptr;
    }

    Texture::~Texture()
    {
        if(IsLoaded())
        {
            Cleanup();
        }
    }

}  // namespace hsk