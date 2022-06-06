#pragma once
#include "../hsk_component.hpp"

namespace hsk {
    class NTexture 
    {

    };

    class TextureStore : public Component
    {
      public:
      protected:
        std::vector<ManagedImage> mTextures;
    };
}  // namespace hsk