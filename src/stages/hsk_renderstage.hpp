#pragma once
#include "../base/hsk_framerenderinfo.hpp"
#include "../base/hsk_vkcontext.hpp"
#include "../hsk_basics.hpp"

namespace hsk {
    class RenderStage
    {
      public:
        RenderStage(){};
        inline virtual void RecordFrame(FrameRenderInfo& renderInfo) {}

        inline virtual void Destroy() {}

      protected:
        const VkContext* mContext;
    };
}  // namespace hsk