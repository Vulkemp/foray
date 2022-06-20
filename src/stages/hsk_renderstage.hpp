#pragma once
#include "../base/hsk_framerenderinfo.hpp"
#include "../base/hsk_vkcontext.hpp"
#include "../hsk_basics.hpp"
#include "../memory/hsk_descriptorsethelper.hpp"
#include "../memory/hsk_managedimage.hpp"

namespace hsk {
    class RenderStage
    {
      public:
        RenderStage(){};
        inline virtual void RecordFrame(FrameRenderInfo& renderInfo) {}
        inline virtual void Destroy() {}

        virtual void OnResized(const VkExtent2D& extent){};

        /// @brief Gets a vector to all color attachments that will be included in a texture array and can be referenced in the shader pass.
        inline std::vector<ManagedImage*>& GetColorAttachments() { return mColorAttachments; }
        inline ManagedImage&               GetDepthAttachment() { return mDepthAttachment; }
        ManagedImage*                      GetColorAttachmentByName(const std::string_view name, bool noThrow = false);


      protected:
        std::vector<ManagedImage*> mColorAttachments;
        ManagedImage               mDepthAttachment;
        const VkContext*           mContext{};
        DescriptorSetHelper        mDescriptorSet;
    };
}  // namespace hsk