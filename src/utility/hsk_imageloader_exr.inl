#pragma once
#include "../hsk_env.hpp"
#include "hsk_imageloader.hpp"
#include <tinyexr/tinyexr.h>

// Disclaimer: Most of the code here is heavily inspired or copied from how Godot engine incorporates the tinyexr image loader

using namespace std::filesystem;

namespace hsk {

    class ExrLoaderCache
    {
      public:
        EXRVersion                                Version{};
        EXRHeader                                 Header{};
        int32_t                                   ChannelIndices[5];
        std::unordered_map<std::string, uint32_t> Channels;

        inline ExrLoaderCache()
        {
            InitEXRHeader(&Header);
            for(int32_t i = 0; i < 5; i++)
            {
                ChannelIndices[i] = -1;
            }
        }
        inline explicit ExrLoaderCache(EXRHeader& header) { Header = header; }

        inline ~ExrLoaderCache() { FreeEXRHeader(&Header); }
    };

    namespace impl {

        template <typename FORMAT_TRAITS>
        void lReadExr(std::vector<uint8_t>& out,
                      const EXRHeader&      header,
                      const EXRImage&       image,
                      const EXRTile*        tiles,
                      uint32_t              numTiles,
                      uint32_t              tileHeight,
                      uint32_t              tileWidth,
                      int32_t               channels[5])
        {
            using component_t     = FORMAT_TRAITS::COMPONENT_TRAITS::COMPONENT;
            const uint32_t strideBytes = FORMAT_TRAITS::STRIDE;
            const uint32_t stride = FORMAT_TRAITS::COMPONENT_COUNT;

            component_t* writeData = reinterpret_cast<component_t*>(out.data());

            for(int tile_index = 0; tile_index < numTiles; tile_index++)
            {
                const EXRTile& tile = tiles[tile_index];
                int            tw   = tile.width;
                int            th   = tile.height;

                const component_t* readStart_r = nullptr;
                const component_t* readStart_g = nullptr;
                const component_t* readStart_b = nullptr;
                const component_t* readStart_a = nullptr;

                if(channels[(int)EImageChannel::R] >= 0)
                {
                    readStart_r = reinterpret_cast<const component_t*>(tile.images[channels[(int)EImageChannel::R]]);
                }
                if(channels[(int)EImageChannel::G] >= 0)
                {
                    readStart_g = reinterpret_cast<const component_t*>(tile.images[channels[(int)EImageChannel::G]]);
                }
                if(channels[(int)EImageChannel::B] >= 0)
                {
                    readStart_b = reinterpret_cast<const component_t*>(tile.images[channels[(int)EImageChannel::B]]);
                }
                if(channels[(int)EImageChannel::A] >= 0)
                {
                    readStart_a = reinterpret_cast<const component_t*>(tile.images[channels[(int)EImageChannel::A]]);
                }

                component_t* writeStart = writeData + (tile.offset_y * tileHeight * image.width + tile.offset_x * tileWidth) * stride;

                for(int y = 0; y < th; y++)
                {
                    const component_t* readRowPos_r = nullptr;
                    const component_t* readRowPos_g = nullptr;
                    const component_t* readRowPos_b = nullptr;
                    const component_t* readRowPos_a = nullptr;
                    if(readStart_r)
                    {
                        readRowPos_r = readStart_r + y * tw;
                    }
                    if(readStart_g)
                    {
                        readRowPos_g = readStart_g + y * tw;
                    }
                    if(readStart_b)
                    {
                        readRowPos_b = readStart_b + y * tw;
                    }
                    if(readStart_a)
                    {
                        readRowPos_a = readStart_a + y * tw;
                    }

                    component_t* writeRowPos = writeStart + (y * image.width * stride);

                    for(int x = 0; x < tw; x++)
                    {
                        component_t r = 0;
                        if(readRowPos_r)
                        {
                            r = *readRowPos_r++;
                        }
                        component_t g = 0;
                        if(readRowPos_g)
                        {
                            g = *readRowPos_g++;
                        }
                        component_t b = 0;
                        if(readRowPos_b)
                        {
                            b = *readRowPos_b++;
                        }
                        component_t a = FORMAT_TRAITS::COMPONENT_TRAITS::ALPHA_FALLBACK;
                        if(readRowPos_a)
                        {
                            a = *readRowPos_a++;
                        }
                        FORMAT_TRAITS::WriteColor(writeRowPos, r, g, b, a);
                        writeRowPos += stride;
                    }
                }
            }
        }

        void DeleteExrLoaderCache(void* loaderCache)
        {
            delete reinterpret_cast<ExrLoaderCache*>(loaderCache);
        }

    }  // namespace impl

    template <VkFormat FORMAT>
    bool ImageLoader<FORMAT>::PopulateImageInfo_TinyExr()
    {
        const char* exrError = nullptr;
        using namespace impl;

        mCustomLoaderInfo        = new ExrLoaderCache();
        mCustomLoaderInfoDeleter = DeleteExrLoaderCache;
        auto& loaderCache        = *(reinterpret_cast<ExrLoaderCache*>(mCustomLoaderInfo));

        // STEP #1: Get EXR version and header information

        bool success = true;
        success      = success && ParseEXRVersionFromFile(&loaderCache.Version, mInfo.Utf8Path.c_str()) == TINYEXR_SUCCESS;
        success      = success && ParseEXRHeaderFromFile(&loaderCache.Header, &loaderCache.Version, mInfo.Utf8Path.c_str(), &exrError) == TINYEXR_SUCCESS;
        if(!success)
        {
            if(exrError)
            {
                logger()->warn("Failed to read image file \"{}\". Failed to read Exr header. TinyExr error: {}", mInfo.Utf8Path, exrError);
                FreeEXRErrorMessage(exrError);
            }
            else
            {
                logger()->warn("Failed to read image file \"{}\". Failed to read Exr header.", mInfo.Utf8Path);
            }
            return false;
        }


        auto& header = loaderCache.Header;

        auto stringToChannel = std::unordered_map<std::string_view, EImageChannel>({
            {"R", EImageChannel::R},
            {"G", EImageChannel::G},
            {"B", EImageChannel::B},
            {"A", EImageChannel::A},
            {"Y", EImageChannel::Y},
        });

        // STEP #2: Process header information
        // Map channel names to indices

        if(header.num_channels > 0)
        {
            switch(header.pixel_types[0])
            {
                case TINYEXR_PIXELTYPE_UINT:
                    Assert(std::is_same_v<typename FORMAT_TRAITS::COMPONENT_TRAITS, ComponentTraits_UInt32>, "EXR component type is Uint32, loader component type does not match!");
                    break;
                case TINYEXR_PIXELTYPE_HALF:
                    Assert(std::is_same_v<typename FORMAT_TRAITS::COMPONENT_TRAITS, ComponentTraits_Fp16>, "EXR component type is Fp16, loader component type does not match!");
                    break;
                case TINYEXR_PIXELTYPE_FLOAT:
                    Assert(std::is_same_v<typename FORMAT_TRAITS::COMPONENT_TRAITS, ComponentTraits_Fp32>, "EXR component type is Fp32, loader component type does not match!");
                    break;
            }
        }

        for(uint32_t channelIndex = 0; channelIndex < header.num_channels; channelIndex++)
        {
            auto& channel = header.channels[channelIndex];
            if(header.pixel_types[channelIndex] != header.pixel_types[0])
            {
                HSK_THROWFMT("Image Loader: .EXR image \"{}\" has different pixel types per channel, which is not supported", mInfo.Name);
            }
            auto iter = stringToChannel.find(channel.name);
            if(iter != stringToChannel.end())
            {
                EImageChannel channel = iter->second;
                mInfo.Channels.push_back(channel);
                loaderCache.ChannelIndices[(int)channel] = channelIndex;
            }
            else
            {
                mInfo.Channels.push_back(EImageChannel::Unknown);
            }
        }

        mInfo.Valid = true;
        return true;
    }

    template <VkFormat FORMAT>
    bool ImageLoader<FORMAT>::Load_TinyExr()
    {
        using namespace impl;

        const char* exrError = nullptr;
        EXRImage    image{};
        InitEXRImage(&image);

        auto& loaderCache = *(reinterpret_cast<ExrLoaderCache*>(mCustomLoaderInfo));
        auto& header      = loaderCache.Header;
        auto& channelMap  = loaderCache.Channels;

        if(LoadEXRImageFromFile(&image, &header, mInfo.Utf8Path.c_str(), &exrError) != TINYEXR_SUCCESS)
        {
            if(exrError)
            {
                logger()->warn("Failed to read image file \"{}\". Failed to load file. TinyExr error: {}", mInfo.Utf8Path, exrError);
                FreeEXRErrorMessage(exrError);
            }
            else
            {
                logger()->warn("Failed to read image file \"{}\". Failed to load file.", mInfo.Utf8Path);
            }
            return false;
        }

        mInfo.Extent.width  = image.width;
        mInfo.Extent.height = image.height;

        // STEP #4: Move EXR lib data to custom data vector

        // Storing as VkFormat::VK_FORMAT_R16G16B16A16_SFLOAT, so resize to pixelcount * 4 components * x bytes per component (2 or 4)
        mRawData.resize(FORMAT_TRAITS::STRIDE * mInfo.Extent.width * mInfo.Extent.height);

        EXRTile single_image_tile;
        int     num_tiles;
        int     tile_width  = 0;
        int     tile_height = 0;

        const EXRTile* exr_tiles;

        if(!header.tiled)
        {
            // In order to be able to read tiled and non-tiled images the same, "simulate" a massive single tile
            single_image_tile.images   = image.images;
            single_image_tile.width    = image.width;
            single_image_tile.height   = image.height;
            single_image_tile.level_x  = image.width;
            single_image_tile.level_y  = image.height;
            single_image_tile.offset_x = 0;
            single_image_tile.offset_y = 0;

            exr_tiles   = &single_image_tile;
            num_tiles   = 1;
            tile_width  = image.width;
            tile_height = image.height;
        }
        else
        {
            tile_width  = header.tile_size_x;
            tile_height = header.tile_size_y;
            num_tiles   = image.num_tiles;
            exr_tiles   = image.tiles;
        }

        lReadExr<FORMAT_TRAITS>(mRawData, header, image, exr_tiles, num_tiles, tile_height, tile_width, loaderCache.ChannelIndices);

        return true;
    }

}  // namespace hsk