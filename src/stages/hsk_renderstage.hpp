#pragma once
#include "../base/hsk_framerenderinfo.hpp"
#include "../hsk_basics.hpp"

namespace hsk {


    class RenderStage
    {
      public:
        inline virtual void RecordFrame(FrameRenderInfo& renderInfo) {}

        inline virtual void Resize(VkExtent2D size){}

        inline virtual void Destroy() {}

        protected:
        VkDevice mDevice;
        VkExtent2D mRenderResolution;
    };
}  // namespace hsk