#pragma once
#include "../core/foray_commandbuffer.hpp"
#include "../core/foray_managedimage.hpp"
#include "../foray_basics.hpp"
#include "../foray_vulkan.hpp"
#include "foray_imageformattraits.hpp"
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
    template <VkFormat FORMAT>
    class ImageLoader
    {
      public:
        using FORMAT_TRAITS = ImageFormatTraits<FORMAT>;

        class ImageInfo : public NoMoveDefaults
        {
          public:
            using FORMAT_TRAITS = ImageFormatTraits<FORMAT>;

            bool                       Valid     = false;
            std::string                Utf8Path  = "";
            std::string                Extension = "";
            std::string                Name      = "";
            std::vector<EImageChannel> Channels;
            VkExtent2D                 Extent = VkExtent2D{};
        };

        inline ImageLoader() {}

        /// @brief Inits the image loader
        /// @return True if the image exists, can be loaded and the data is suitable for the
        inline bool Init(std::string_view utf8path);

        /// @brief Checks if format the loader was initialized in supports linear tiling transfer and shader read
        inline static bool sFormatSupported(const core::VkContext* context);

        /// @brief Loads the file into CPU memory (Init first!)
        inline bool Load();

        /// @brief Cleans up the loader
        inline void Destroy();

        virtual inline ~ImageLoader() { Destroy(); }

        FORAY_PROPERTY_ALLGET(Info)
        FORAY_PROPERTY_ALLGET(RawData)

        inline void InitManagedImage(const core::VkContext*          context,
                                     core::ManagedImage*             image,
                                     core::ManagedImage::CreateInfo& ci,
                                     VkImageLayout                   afterwrite = VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) const;
        inline void InitManagedImage(const core::VkContext*          context,
                                     core::CommandBuffer&            cmdBuffer,
                                     core::ManagedImage*             image,
                                     core::ManagedImage::CreateInfo& ci,
                                     VkImageLayout                   afterwrite = VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) const;
        inline void UpdateManagedImageCI(core::ManagedImage::CreateInfo& ci) const;
        inline void WriteManagedImageData(core::ManagedImage* image, VkImageLayout afterwrite) const;
        inline void WriteManagedImageData(core::CommandBuffer& cmdBuffer, core::ManagedImage* image, VkImageLayout afterwrite) const;

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

#include "foray_imageloader.inl"