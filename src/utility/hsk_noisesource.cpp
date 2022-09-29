#include "hsk_noisesource.hpp"
#include <random>

namespace hsk {
    NoiseSource::NoiseSource() {}

    void NoiseSource::Create(const VkContext* context)
    {
        const uint32_t EDGE = 2048u;

        ManagedImage::CreateInfo ci("Noise Source", VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED,
                                    VkImageUsageFlagBits::VK_IMAGE_USAGE_TRANSFER_DST_BIT | VkImageUsageFlagBits::VK_IMAGE_USAGE_SAMPLED_BIT, VkFormat::VK_FORMAT_R32_UINT,
                                    VkExtent3D{
                                        .width  = EDGE,
                                        .height = EDGE,
                                        .depth  = 1,
                                    });

        mImage.Destroy();
        mImage.Create(context, ci);

        int32_t               valueCount = EDGE * EDGE;
        std::vector<uint32_t> values     = std::vector<uint32_t>(valueCount);

        std::mt19937_64 rngEngine;
        uint64_t        max = 1ULL << 32;

        for(int32_t i = 0; i < valueCount; i++)
        {
            values[i + 0] = static_cast<uint32_t>(rngEngine() % max);
        }
        mImage.WriteDeviceLocalData(values.data(), sizeof(float) * (size_t)valueCount, VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }
    void NoiseSource::Destroy()
    {
        mImage.Destroy();
    }
    bool NoiseSource::Exists() const
    {
        return mImage.Exists();
    }
}  // namespace hsk
