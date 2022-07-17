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
        std::unordered_map<std::string, uint32_t> Channels;

        inline ExrLoaderCache() { InitEXRHeader(&Header); }
        inline explicit ExrLoaderCache(EXRHeader& header) { Header = header; }

        inline ~ExrLoaderCache() { FreeEXRHeader(&Header); }
    };

    void lReadFp16Exr(std::vector<uint8_t>&                      out,
                      const EXRHeader&                           header,
                      const EXRImage&                            image,
                      const EXRTile*                             tiles,
                      uint32_t                                   numTiles,
                      uint32_t                                   tileHeight,
                      uint32_t                                   tileWidth,
                      std::unordered_map<std::string, uint32_t>& channelMap)
    {
        using fp16_t           = uint16_t;
        uint8_t* writeData     = out.data();
        fp16_t*  writeDataFp16 = reinterpret_cast<uint16_t*>(writeData);

        for(int tile_index = 0; tile_index < numTiles; tile_index++)
        {
            const EXRTile& tile = tiles[tile_index];

            int tw = tile.width;
            int th = tile.height;

            const fp16_t* r_channel_start = reinterpret_cast<const fp16_t*>(tile.images[channelMap["R"]]);
            const fp16_t* g_channel_start = reinterpret_cast<const fp16_t*>(tile.images[channelMap["G"]]);
            const fp16_t* b_channel_start = reinterpret_cast<const fp16_t*>(tile.images[channelMap["B"]]);
            const fp16_t* a_channel_start = nullptr;
            if(channelMap.contains("A"))
            {
                a_channel_start = reinterpret_cast<const fp16_t*>(tile.images[channelMap["A"]]);
            }

            fp16_t* first_row_w16 = writeDataFp16 + (tile.offset_y * tileHeight * image.width + tile.offset_x * tileWidth) * 4;

            for(int y = 0; y < th; y++)
            {
                const fp16_t* r_channel = r_channel_start + y * tileWidth;
                const fp16_t* g_channel = g_channel_start + y * tileWidth;
                const fp16_t* b_channel = b_channel_start + y * tileWidth;
                const fp16_t* a_channel = nullptr;
                if(a_channel_start)
                {
                    a_channel = a_channel_start + y * tileWidth;
                }

                fp16_t* row_w = first_row_w16 + (y * image.width * 4);

                for(int x = 0; x < tw; x++)
                {
                    fp16_t r = *r_channel++;
                    fp16_t g = *g_channel++;
                    fp16_t b = *b_channel++;
                    fp16_t a = 0x3C00;  // float 16 representation of 1.0
                    if(a_channel)
                    {
                        a = *a_channel++;
                    }

                    *row_w++ = r;
                    *row_w++ = g;
                    *row_w++ = b;
                    *row_w++ = a;
                }
            }
        }
    }

    void lReadFp32Exr(std::vector<uint8_t>&                      out,
                      const EXRHeader&                           header,
                      const EXRImage&                            image,
                      const EXRTile*                             tiles,
                      uint32_t                                   numTiles,
                      uint32_t                                   tileHeight,
                      uint32_t                                   tileWidth,
                      std::unordered_map<std::string, uint32_t>& channelMap)
    {
        uint8_t* writeData     = out.data();
        fp32_t*  writeDataFp32 = reinterpret_cast<fp32_t*>(writeData);

        for(int tile_index = 0; tile_index < numTiles; tile_index++)
        {
            const EXRTile& tile = tiles[tile_index];

            int tw = tile.width;
            int th = tile.height;

            const fp32_t* r_channel_start = reinterpret_cast<const fp32_t*>(tile.images[channelMap["R"]]);
            const fp32_t* g_channel_start = reinterpret_cast<const fp32_t*>(tile.images[channelMap["G"]]);
            const fp32_t* b_channel_start = reinterpret_cast<const fp32_t*>(tile.images[channelMap["B"]]);
            const fp32_t* a_channel_start = nullptr;
            if(channelMap.contains("A"))
            {
                a_channel_start = reinterpret_cast<const float*>(tile.images[channelMap["A"]]);
            }

            fp32_t* first_row_w32 = writeDataFp32 + (tile.offset_y * tileHeight * image.width + tile.offset_x * tileWidth) * 4;

            for(int y = 0; y < th; y++)
            {
                const fp32_t* r_channel = r_channel_start + y * tileWidth;
                const fp32_t* g_channel = g_channel_start + y * tileWidth;
                const fp32_t* b_channel = b_channel_start + y * tileWidth;
                const fp32_t* a_channel = nullptr;
                if(a_channel_start)
                {
                    a_channel = a_channel_start + y * tileWidth;
                }

                fp32_t* row_w = first_row_w32 + (y * image.width * 4);

                for(int x = 0; x < tw; x++)
                {
                    fp32_t r = *r_channel++;
                    fp32_t g = *g_channel++;
                    fp32_t b = *b_channel++;
                    fp32_t a = 1.f;
                    if(a_channel)
                    {
                        a = *a_channel++;
                    }

                    *row_w++ = r;
                    *row_w++ = g;
                    *row_w++ = b;
                    *row_w++ = a;
                }
            }
        }
    }

    void DeleteExrLoaderCache(void* loaderCache)
    {
        delete reinterpret_cast<ExrLoaderCache*>(loaderCache);
    }

    bool ImageLoader::sPopulateImageInfo_TinyExr(ImageInfo& out)
    {
        const char* exrError = nullptr;

        out.CustomLoaderInfo        = new ExrLoaderCache();
        out.CustomLoaderInfoDeleter = DeleteExrLoaderCache;
        auto& loaderCache           = *(reinterpret_cast<ExrLoaderCache*>(out.CustomLoaderInfo));

        // STEP #1: Get EXR version and header information

        bool success = true;
        success      = success && ParseEXRVersionFromFile(&loaderCache.Version, out.Utf8Path.c_str()) == TINYEXR_SUCCESS;
        success      = success && ParseEXRHeaderFromFile(&loaderCache.Header, &loaderCache.Version, out.Utf8Path.c_str(), &exrError) == TINYEXR_SUCCESS;
        if(!success)
        {
            if(exrError)
            {
                logger()->warn("Failed to read image file \"{}\". Failed to read Exr header. TinyExr error: {}", out.Utf8Path, exrError);
                FreeEXRErrorMessage(exrError);
            }
            else
            {
                logger()->warn("Failed to read image file \"{}\". Failed to read Exr header.", out.Utf8Path);
            }
            return false;
        }

        auto& header   = loaderCache.Header;
        auto& channels = loaderCache.Channels;

        // STEP #2: Process header information

        // Detect use of fp16
        bool isFp16 = false;
        // Map channel names to indices

        for(uint32_t channelIndex = 0; channelIndex < header.num_channels; channelIndex++)
        {
            auto& channel          = header.channels[channelIndex];
            channels[channel.name] = channelIndex;
            if(header.pixel_types[channelIndex] == TINYEXR_PIXELTYPE_HALF)
            {
                isFp16                                       = true;
                channels[header.channels[channelIndex].name] = channelIndex;
            }
        }
        out.PixelComponent     = isFp16 ? EPixelComponent::Fp16 : EPixelComponent::Fp32;
        out.PixelComponentSize = isFp16 ? 2U : 4U;
        out.HasAlphaChannel    = channels.contains("A");
        out.IsGrayScale        = channels.contains("Y");

        if(!out.IsGrayScale && (!channels.contains("R") || !channels.contains("G") || !channels.contains("B")))
        {
            // No channel types we can use
            logger()->warn("Failed to read image file \"{}\". Missing channels for full RGB.");
            return false;
        }

        out.Valid = true;
        return true;
    }

    bool ImageLoader::Load_TinyExr()
    {
        const char* exrError = nullptr;
        EXRImage    image{};
        InitEXRImage(&image);

        auto& loaderCache = *(reinterpret_cast<ExrLoaderCache*>(mInfo.CustomLoaderInfo));
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

        // STEP #4: Move EXR lib data to custom data vector

        std::vector<uint8_t> data;
        // Storing as VkFormat::VK_FORMAT_R16G16B16A16_SFLOAT, so resize to pixelcount * 4 components * x bytes per component (2 or 4)
        data.resize(image.width * image.height * 4 * mInfo.PixelComponentSize);

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

        if(mInfo.PixelComponentSize == 2)
        {
            lReadFp16Exr(mRawData, header, image, exr_tiles, num_tiles, tile_height, tile_width, channelMap);
        }
        else
        {
            lReadFp32Exr(mRawData, header, image, exr_tiles, num_tiles, tile_height, tile_width, channelMap);
        }

        return true;
    }

}  // namespace hsk