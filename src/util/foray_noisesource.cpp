#include "foray_noisesource.hpp"
#include "../core/foray_samplercollection.hpp"
#include <random>

namespace foray::util {
    NoiseSource::NoiseSource()
    {
        mSampler.SetManagedImage(&mImage);
    }

    void NoiseSource::Create(core::Context* context)
    {
        const uint32_t EDGE = 2048u;

        VkImageUsageFlags usage = VkImageUsageFlagBits::VK_IMAGE_USAGE_TRANSFER_DST_BIT | VkImageUsageFlagBits::VK_IMAGE_USAGE_SAMPLED_BIT;

        core::ManagedImage::CreateInfo ci(usage, VkFormat::VK_FORMAT_R32_UINT, VkExtent2D{.width = EDGE, .height = EDGE}, "Noise Source");

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

        VkSamplerCreateInfo samplerCi{.sType                   = VkStructureType::VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
                                      .magFilter               = VkFilter::VK_FILTER_NEAREST,
                                      .minFilter               = VkFilter::VK_FILTER_NEAREST,
                                      .addressModeU            = VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_REPEAT,
                                      .addressModeV            = VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_REPEAT,
                                      .addressModeW            = VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_REPEAT,
                                      .anisotropyEnable        = VK_FALSE,
                                      .compareEnable           = VK_FALSE,
                                      .minLod                  = 0,
                                      .maxLod                  = 0,
                                      .unnormalizedCoordinates = VK_FALSE};

        mSampler.Init(context, samplerCi);
    }
    void NoiseSource::Destroy()
    {
        mSampler.Destroy();
        mImage.Destroy();
    }
    bool NoiseSource::Exists() const
    {
        return mImage.Exists();
    }
}  // namespace foray::util
