#include "foray_noisesource.hpp"
#include "../core/foray_samplercollection.hpp"
#include <random>

namespace foray::util {

    void NoiseSource::Create(core::Context* context, uint32_t edge, uint32_t depth)
    {
        VkImageUsageFlags usage = VkImageUsageFlagBits::VK_IMAGE_USAGE_TRANSFER_DST_BIT | VkImageUsageFlagBits::VK_IMAGE_USAGE_SAMPLED_BIT | VkImageUsageFlagBits::VK_IMAGE_USAGE_STORAGE_BIT;

        core::ManagedImage::CreateInfo ci(usage, VkFormat::VK_FORMAT_R32_UINT, VkExtent2D{.width = edge, .height = edge}, "Noise Source");
        ci.ImageCI.extent.depth = depth;

        mImage.Destroy();
        mImage.Create(context, ci);

        Regenerate();
    }
    void NoiseSource::Regenerate()
    {
        int32_t               valueCount = mImage.GetExtent3D().width * mImage.GetExtent3D().height * mImage.GetExtent3D().depth;
        std::vector<uint32_t> values     = std::vector<uint32_t>(valueCount);

        std::mt19937_64 rngEngine;
        uint64_t        max = 1ULL << 32;

        for(int32_t i = 0; i < valueCount; i++)
        {
            values[i + 0] = static_cast<uint32_t>(rngEngine() % max);
        }
        mImage.WriteDeviceLocalData(values.data(), sizeof(float) * (size_t)valueCount, VkImageLayout::VK_IMAGE_LAYOUT_GENERAL);
    }
    void NoiseSource::Destroy()
    {
        mImage.Destroy();
    }
    bool NoiseSource::Exists() const
    {
        return mImage.Exists();
    }
}  // namespace foray::util
