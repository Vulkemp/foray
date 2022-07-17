#pragma once
#include "../hsk_basics.hpp"
#include <functional>

namespace hsk {

    /// @brief General purpose image loader
    class ImageLoader
    {
      public:
        enum class EPixelComponent
        {
            Fp16,
            Fp32,
            Int16,
            UInt8,
        };

        class ImageInfo : public NoMoveDefaults
        {
          public:
            bool                   Valid                   = false;
            std::string            Utf8Path                = "";
            std::string_view       Extension               = "";
            std::string_view       Name                    = "";
            EPixelComponent        PixelComponent          = EPixelComponent::Fp16;
            uint32_t               PixelComponentSize      = 0;
            VkExtent2D             Extent                  = VkExtent2D{};
            bool                   IsGrayScale             = false;
            bool                   HasAlphaChannel         = false;
            void*                  CustomLoaderInfo        = nullptr;
            std::function<void(void*)> CustomLoaderInfoDeleter = {};

            inline ImageInfo() {}

            virtual ~ImageInfo();

            VkFormat sGetDefaultFormat() const;
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
}  // namespace hsk