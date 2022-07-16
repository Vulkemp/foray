#pragma once
#include "../base/hsk_framerenderinfo.hpp"
#include "../base/hsk_vkcontext.hpp"
#include "../hsk_basics.hpp"
#include "../memory/hsk_descriptorsethelper.hpp"
#include "../memory/hsk_managedimage.hpp"
#include "hsk_stagereferences.hpp"

namespace hsk {
    class RenderStage
    {
      public:
        RenderStage(){};
        inline virtual void RecordFrame(FrameRenderInfo& renderInfo) {}
        inline virtual void Destroy() { mDescriptorSet.Cleanup(); }

        virtual void OnResized(const VkExtent2D& extent){};

        /// @brief Gets a vector to all color attachments that will be included in a texture array and can be referenced in the shader pass.
        inline std::vector<ManagedImage*>& GetColorAttachments() { return mColorAttachments; }
        inline ManagedImage&               GetDepthAttachment() { return mDepthAttachment; }
        ManagedImage*                      GetColorAttachmentByName(const std::string_view name, bool noThrow = false);

        inline virtual const std::vector<ResourceReferenceBase*>& Depends() const { return {}; }
        inline virtual const std::vector<ResourceReferenceBase*>& Provides() const { return {}; }

        inline virtual std::string_view GetName() const { return ""; }

      protected:
        std::vector<ManagedImage*> mColorAttachments;
        ManagedImage               mDepthAttachment;
        const VkContext*           mContext{};
        DescriptorSetHelper        mDescriptorSet;
    };
}  // namespace hsk