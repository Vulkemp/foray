#pragma once
#include "../osi/foray_env.hpp"
#include "foray_imageloader.hpp"
#include <tinygltf/stb_image.h>
#include "../foray_logger.hpp"

namespace foray::util {

    namespace impl {
        struct StbLoaderCache
        {
            bool Is16bit = false;
            bool IsHdr   = false;
        };

    }  // namespace impl

    template <VkFormat FORMAT>
    bool ImageLoader<FORMAT>::PopulateImageInfo_Stb()
    {
        using namespace impl;

        int  width           = 0;
        int  height          = 0;
        int  component_count = 0;
        bool gotInfo         = !!stbi_info(mInfo.Utf8Path.c_str(), &width, &height, &component_count);

        if(!gotInfo)
        {
            return false;
        }

        // Genuinely illegal: reinterprete customloaderinfo pointer (guaranteed at minimum 32bit) to store the loader cache struct (16 bit).
        // The deleter is never set, so all other code will ignore what is stored here
        StbLoaderCache& cache = *reinterpret_cast<StbLoaderCache*>(&mCustomLoaderInfo);
        new(&cache) StbLoaderCache();

        cache.Is16bit = !!stbi_is_16_bit(mInfo.Utf8Path.c_str());
        cache.IsHdr   = !!stbi_is_hdr(mInfo.Utf8Path.c_str());

        switch(component_count)
        {
            case 1:
                mInfo.Channels = {EImageChannel::Y};
                break;
            case 2:
                mInfo.Channels = {EImageChannel::Y, EImageChannel::A};
                break;
            case 3:
                mInfo.Channels = {EImageChannel::R, EImageChannel::G, EImageChannel::B};
                break;
            case 4:
                mInfo.Channels = {EImageChannel::R, EImageChannel::G, EImageChannel::B, EImageChannel::A};
                break;
        }
        mInfo.Extent.width  = static_cast<uint32_t>(width);
        mInfo.Extent.height = static_cast<uint32_t>(height);

        if constexpr(FORMAT_TRAITS::COMPONENT_TRAITS::IS_FLOAT && FORMAT_TRAITS::COMPONENT_TRAITS::SIZE != 4)
        {
            logger()->warn("ImageLoad: Stb image loader does not support half or double precision floating point values!");
            return false;
        }


        mInfo.Valid = width > 0 && height > 0 && !!mInfo.Channels.size();
        return mInfo.Valid;
    }

    namespace impl {
        template <typename FORMAT_TRAITS>
        uint8_t* lReadStbUint8(const char* name, int desired_channels)
        {
            int width            = 0;
            int height           = 0;
            int channels_in_file = 0;
            return reinterpret_cast<uint8_t*>(stbi_load(name, &width, &height, &channels_in_file, desired_channels));
        }
        template <typename FORMAT_TRAITS>
        uint8_t* lReadStbUint16(const char* name, int desired_channels)
        {
            int width            = 0;
            int height           = 0;
            int channels_in_file = 0;
            return reinterpret_cast<uint8_t*>(stbi_load_16(name, &width, &height, &channels_in_file, desired_channels));
        }
        template <typename FORMAT_TRAITS>
        uint8_t* lReadStbFp32(const char* name, int desired_channels)
        {
            int width            = 0;
            int height           = 0;
            int channels_in_file = 0;
            return reinterpret_cast<uint8_t*>(stbi_loadf(name, &width, &height, &channels_in_file, desired_channels));
        }
    }  // namespace impl

    template <VkFormat FORMAT>
    bool ImageLoader<FORMAT>::Load_Stb()
    {
        using namespace impl;
        StbLoaderCache& cache = *reinterpret_cast<StbLoaderCache*>(&mCustomLoaderInfo);

        uint8_t*    stbdata = nullptr;
        const char* name    = mInfo.Utf8Path.c_str();

        int desired_channels = static_cast<int>(FORMAT_TRAITS::COMPONENT_COUNT);

        if constexpr(FORMAT_TRAITS::COMPONENT_TRAITS::IS_FLOAT)
        {
            stbdata = lReadStbFp32<FORMAT_TRAITS>(name, desired_channels);
        }
        if constexpr(FORMAT_TRAITS::COMPONENT_TRAITS::SIZE == 1)
        {
            stbdata = lReadStbUint8<FORMAT_TRAITS>(name, desired_channels);
        }
        if constexpr(FORMAT_TRAITS::COMPONENT_TRAITS::SIZE == 2)
        {
            stbdata = lReadStbUint16<FORMAT_TRAITS>(name, desired_channels);
        }

        if(!stbdata)
        {
            return false;
        }

        mRawData.resize(FORMAT_TRAITS::BYTESTRIDE * mInfo.Extent.width * mInfo.Extent.height);

        memcpy(mRawData.data(), stbdata, mRawData.size());
        stbi_image_free(stbdata);
        return true;
    }

}  // namespace foray