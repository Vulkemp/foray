#pragma once
#include "../hsk_basics.hpp"
#include "../memory/hsk_managedimage.hpp"

namespace hsk {
    class SkyboxTexture : public DeviceResourceBase
    {
      public:

        virtual bool InitFromFile(const VkContext* context, std::string_view utf8path);

        virtual bool Exists() const override { return mImage.Exists(); }
        virtual void Cleanup() override { mImage.Cleanup(); }

      private:
        ManagedImage mImage;
    };
}  // namespace hsk