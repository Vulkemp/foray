#include "hsk_skybox.hpp"
#include "../utility/hsk_imageloader.hpp"

namespace hsk {
    bool SkyboxTexture::InitFromFile(const VkContext* context, std::string_view utf8path)
    {
        ImageLoader loader;
        if(!loader.InitImageInfo(utf8path))
        {
            return false;
        }

        if(!loader.Load())
        {
            return false;
        }

        auto& info = loader.GetInfo();
        auto& data = loader.GetRawData();

        VkFormat   format = info.PixelComponent == ImageLoader::EPixelComponent::Fp16 ? VkFormat::VK_FORMAT_R16G16B16A16_SFLOAT : VkFormat::VK_FORMAT_R32G32B32A32_SFLOAT;
        VkExtent3D extent{.width = info.Extent.width, .height = info.Extent.height, .depth = 1};

        mImage.Create(context, VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, 0, extent, VkImageUsageFlagBits::VK_IMAGE_USAGE_SAMPLED_BIT, format,
                      VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT);
        mImage.WriteDeviceLocalData(data.data(), data.size(), VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        return true;
    }
}  // namespace hsk