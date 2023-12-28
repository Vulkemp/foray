#pragma once
#include "../core/commandbuffer.hpp"
#include "../core/image.hpp"
#include "../basics.hpp"
#include "../vulkan.hpp"
#include "../osi/path.hpp"
#include "imageformattraits.hpp"
#include <functional>

namespace foray::util {

    enum class EImageChannel
    {
        Unknown = -1,
        R       = 0,
        G,
        B,
        A,
        Y
    };

    /// @brief General purpose image loader
    /// @details
    /// # Supported File Types
    ///  * PNG, JPG, BMP, HDR via StbImage (plus a few other, see <tinygltf/stb_image.h>)
    ///  * EXR via TinyExr
    template <vk::Format FORMAT>
    class ImageLoader
    {
      public:
        using FORMAT_TRAITS = ImageFormatTraits<FORMAT>;

        class ImageInfo : public NoMoveDefaults
        {
          public:
            bool                       Valid = false;
            osi::Utf8Path              Utf8Path;
            std::string                Extension = "";
            std::string                Name      = "";
            std::vector<EImageChannel> Channels;
            VkExtent2D                 Extent = VkExtent2D{};
        };

        inline ImageLoader() {}

        /// @brief Inits the image loader
        /// @return True if the image exists, can be loaded and the data is suitable for the
        inline bool Init(const osi::Utf8Path& utf8path);

        /// @brief Checks if format the loader was initialized in supports linear tiling transfer and shader read
        inline static bool sFormatSupported(core::Context* context);

        /// @brief Loads the file into CPU memory (Init first!)
        inline bool Load();

        /// @brief Cleans up the loader
        inline void Destroy();

        virtual inline ~ImageLoader() { Destroy(); }

        FORAY_GETTER_CR(Info)
        FORAY_GETTER_CR(RawData)

        inline void UpdateManagedImageCI(core::Image::CreateInfo& ci) const;
        inline void WriteManagedImageData(core::Image* image, vk::ImageLayout afterwrite = vk::ImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) const;
        inline void WriteManagedImageData(core::HostSyncCommandBuffer& cmdBuffer, core::Image* image, vk::ImageLayout afterwrite = vk::ImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) const;

      protected:
        ImageInfo                  mInfo;
        std::vector<uint8_t>       mRawData;
        void*                      mCustomLoaderInfo        = nullptr;
        std::function<void(void*)> mCustomLoaderInfoDeleter = {};

        bool PopulateImageInfo_TinyExr();
        bool PopulateImageInfo_Stb();
        bool Load_TinyExr();
        bool Load_Stb();
    };

}  // namespace foray::util

#include "imageloader.inl"