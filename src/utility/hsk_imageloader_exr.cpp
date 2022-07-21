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

    template <typename component_t>
    void lReadExr(std::vector<uint8_t>&                               out,
                  const EXRHeader&                                    header,
                  const EXRImage&                                     image,
                  const EXRTile*                                      tiles,
                  uint32_t                                            numTiles,
                  uint32_t                                            tileHeight,
                  uint32_t                                            tileWidth,
                  std::vector<ImageLoader::ImageInfo::ChannelConfig>& channels,
                  uint32_t                                            channelStride)
    {
        component_t* writeData = reinterpret_cast<component_t*>(out.data());

        auto channelToString = std::unordered_map<ImageLoader::EChannel, const char*>({
            {ImageLoader::EChannel::R, "R"},
            {ImageLoader::EChannel::G, "G"},
            {ImageLoader::EChannel::B, "B"},
            {ImageLoader::EChannel::A, "A"},
            {ImageLoader::EChannel::Y, "Y"},
        });


        for(int tile_index = 0; tile_index < numTiles; tile_index++)
        {
            const EXRTile& tile = tiles[tile_index];
            int            tw   = tile.width;
            int            th   = tile.height;

            int channelWriteOffset = 0;
            for(int channel_index = 0; channel_index < channels.size(); channel_index++)
            {
                if(!channels[channel_index].Transfer)
                {
                    continue;
                }

                const component_t* readStart = reinterpret_cast<const component_t*>(tile.images[channel_index]);

                component_t* writeStart = writeData + (tile.offset_y * tileHeight * image.width + tile.offset_x * tileWidth) * channelStride;

                for(int y = 0; y < th; y++)
                {
                    const component_t* readRowPos = readStart + y * tw;

                    component_t* writeRowPos = writeStart + (y * image.width * channelStride);

                    for(int x = 0; x < tw; x++)
                    {
                        component_t v = *readRowPos;
                        readRowPos++;
                        *writeRowPos = v;
                        writeRowPos += channelStride;
                    }
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

        auto& header = loaderCache.Header;
        // auto& channels = loaderCache.Channels;

        auto stringToChannel = std::unordered_map<std::string_view, EChannel>({
            {"R", EChannel::R},
            {"G", EChannel::G},
            {"B", EChannel::B},
            {"A", EChannel::A},
            {"Y", EChannel::Y},
        });

        // STEP #2: Process header information
        // Map channel names to indices

        if(header.num_channels > 0)
        {
            switch(header.pixel_types[0])
            {
                case TINYEXR_PIXELTYPE_UINT:
                    out.PixelComponent = EPixelComponent::UInt32;
                    break;
                case TINYEXR_PIXELTYPE_HALF:
                    out.PixelComponent = EPixelComponent::Fp16;
                    break;
                case TINYEXR_PIXELTYPE_FLOAT:
                    out.PixelComponent = EPixelComponent::Fp32;
                    break;
            }
        }

        for(uint32_t channelIndex = 0; channelIndex < header.num_channels; channelIndex++)
        {
            auto& channel = header.channels[channelIndex];
            if(header.pixel_types[channelIndex] != header.pixel_types[0])
            {
                HSK_THROWFMT("Image Loader: .EXR image \"{}\" has different pixel types per channel, which is not supported", out.Name);
            }
            auto iter = stringToChannel.find(channel.name);
            if(iter != stringToChannel.end())
            {
                ImageInfo::ChannelConfig config{.Channel = iter->second, .Transfer = true};
                out.Channels.push_back(config);
                if(config.Channel == EChannel::A)
                {
                    out.HasAlphaChannel = true;
                }
                else if(config.Channel == EChannel::Y)
                {
                    out.IsGrayScale = true;
                }
            }
            else
            {
                ImageInfo::ChannelConfig config{.Channel = EChannel::Unknown, .Transfer = false};
                out.Channels.push_back(config);
            }
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

        // Storing as VkFormat::VK_FORMAT_R16G16B16A16_SFLOAT, so resize to pixelcount * 4 components * x bytes per component (2 or 4)
        mRawData.resize(mInfo.NaiveDataSize());

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

        switch(mInfo.PixelComponent)
        {
            case EPixelComponent::Fp16:
                lReadExr<uint16_t>(mRawData, header, image, exr_tiles, num_tiles, tile_height, tile_width, mInfo.Channels, mInfo.TransferChannelCount());
                break;
            case EPixelComponent::Fp32:
                lReadExr<fp32_t>(mRawData, header, image, exr_tiles, num_tiles, tile_height, tile_width, mInfo.Channels, mInfo.TransferChannelCount());
                break;
            case EPixelComponent::UInt32:
                lReadExr<uint32_t>(mRawData, header, image, exr_tiles, num_tiles, tile_height, tile_width, mInfo.Channels, mInfo.TransferChannelCount());
                break;
        }

        return true;
    }

}  // namespace hsk