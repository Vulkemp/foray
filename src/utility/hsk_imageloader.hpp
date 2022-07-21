#pragma once
#include "../hsk_basics.hpp"
#include <functional>

namespace hsk {

    /// @brief General purpose image loader
    class ImageLoader
    {
      public:
        /// @brief Component types
        enum class EPixelComponent
        {
            // 16 bit float
            Fp16,
            // 32 bit float
            Fp32,
            // 8 bit unsigned int
            UInt8,
            // 8 bit signed int
            Int8,
            Int16,
            UInt16,
            Int32,
            UInt32
        };

        enum class EChannel
        {
            Unknown = -1,
            R       = 0,
            G,
            B,
            A,
            Y
        };

        class ImageInfo : public NoMoveDefaults
        {
          public:
            struct ChannelConfig
            {
              public:
                EChannel Channel;
                bool     Transfer;
            };

            bool                       Valid          = false;
            std::string                Utf8Path       = "";
            std::string_view           Extension      = "";
            std::string_view           Name           = "";
            EPixelComponent            PixelComponent = EPixelComponent::Fp16;
            std::vector<ChannelConfig> Channels;
            VkExtent2D                 Extent                  = VkExtent2D{};
            bool                       IsGrayScale             = false;
            bool                       HasAlphaChannel         = false;
            void*                      CustomLoaderInfo        = nullptr;
            std::function<void(void*)> CustomLoaderInfoDeleter = {};

            inline ImageInfo() {}

            virtual ~ImageInfo();

            inline uint32_t GetPixelComponentSize() const;
            VkFormat        sGetDefaultFormat() const;
            inline size_t   NaiveDataSize() const;
            inline uint32_t TransferChannelCount() const;
        };

        static bool sGetImageInfo(std::string_view utf8path, ImageInfo& out);

        inline bool InitImageInfo(std::string_view utf8path) { return sGetImageInfo(utf8path, mInfo); }

        inline ImageLoader() {}
        inline explicit ImageLoader(const ImageInfo& info) {}
        inline explicit ImageLoader(std::string_view utf8path) {}

        bool Load();

        virtual inline ~ImageLoader() {}

        HSK_PROPERTY_ALLGET(Info)
        HSK_PROPERTY_ALLGET(RawData)

      protected:
        ImageInfo            mInfo;
        std::vector<uint8_t> mRawData;

        static bool sPopulateImageInfo_TinyExr(ImageInfo& out);
        static bool sPopulateImageInfo_StbHdr(ImageInfo& out);
        static bool sPopulateImageInfo_StbLdr(ImageInfo& out);
        bool        Load_TinyExr();
        bool        Load_StbHdr();
        bool        Load_StbLdr();
    };

    inline uint32_t ImageLoader::ImageInfo::GetPixelComponentSize() const
    {
        switch(PixelComponent)
        {
            case EPixelComponent::UInt8:
            case EPixelComponent::Int8:
                return 1;
            case EPixelComponent::Int16:
            case EPixelComponent::UInt16:
            case EPixelComponent::Fp16:
                return 2;
            case EPixelComponent::Int32:
            case EPixelComponent::UInt32:
            case EPixelComponent::Fp32:
                return 4;
            default:
                return 0;
        }
    }
    inline size_t ImageLoader::ImageInfo::NaiveDataSize() const
    {
        return Extent.width * Extent.height * TransferChannelCount() * GetPixelComponentSize();
    }
    inline uint32_t ImageLoader::ImageInfo::TransferChannelCount() const
    {
        uint32_t result = 0;
        for(auto& channel : Channels)
        {
            if(channel.Transfer)
            {
                result++;
            }
        }
        return result;
    }
}  // namespace hsk