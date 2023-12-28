#include "noisesource.hpp"
#include "../core/samplercollection.hpp"
#include <random>

namespace foray::util {

    NoiseSource::NoiseSource(core::Context* context, uint32_t edge, uint32_t depth)
    {
        VkImageUsageFlags usage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eStorage;

        core::Image::CreateInfo ci = core::Image::CreateInfo::PresetTexture(vk::Format::VK_FORMAT_R32_UINT, VkExtent2D{edge, edge, depth}, 1u);
        ci.AddUsageFlagsBits(vk::ImageUsageFlagBits::eStorage).SetName("Noise Source");

        mImage.New(context, ci);

        Regenerate();
    }
    void NoiseSource::Regenerate()
    {
        int32_t               valueCount = mImage->GetExtent().width * mImage->GetExtent().height * mImage->GetExtent().depth;
        std::vector<uint32_t> values     = std::vector<uint32_t>(valueCount);

        std::mt19937_64 rngEngine;
        uint64_t        max = 1ULL << 32;

        for(int32_t i = 0; i < valueCount; i++)
        {
            values[i + 0] = static_cast<uint32_t>(rngEngine() % max);
        }
        mImage->WriteDeviceLocalData(values.data(), sizeof(float) * (size_t)valueCount, vk::ImageLayout::VK_IMAGE_LAYOUT_GENERAL);
    }
}  // namespace foray::util
