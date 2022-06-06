#pragma once
#include "../hsk_component.hpp"
#include "../../memory/hsk_managedimage.hpp"

namespace hsk {
    class TextureStore : public Component
    {
      public:

        void Cleanup();

        virtual ~TextureStore() {}

        HSK_PROPERTY_CGET(Textures)
        HSK_PROPERTY_GET(Textures)

      protected:
        std::vector<std::unique_ptr<ManagedImage>> mTextures;
    };
}  // namespace hsk