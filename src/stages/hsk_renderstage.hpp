#pragma once
#include "../base/hsk_framerenderinfo.hpp"
#include "../base/hsk_vkcontext.hpp"
#include "../hsk_basics.hpp"
#include "../memory/hsk_descriptorsethelper.hpp"
#include "../memory/hsk_managedimage.hpp"
#include "hsk_stagereferences.hpp"
#include "../base/hsk_shadercompiler.hpp"

namespace hsk {

    class RenderStage
    {
      public:
        RenderStage(){};
        inline virtual void RecordFrame(FrameRenderInfo& renderInfo) {}

        inline virtual void Destroy()
        {
            DestroyResolutionDependentComponents();
            DestroyFixedComponents();
            mDescriptorSet.Destroy();
        }


        virtual void OnResized(const VkExtent2D& extent)
        {
            DestroyResolutionDependentComponents();
            CreateResolutionDependentComponents();
        };

        /// @brief Gets a vector to all color attachments that will be included in a texture array and can be referenced in the shader pass.
        inline std::vector<ManagedImage*>& GetColorAttachments() { return mColorAttachments; }
        ManagedImage*                      GetColorAttachmentByName(const std::string_view name, bool noThrow = false);

        inline virtual const std::vector<ResourceReferenceBase*>& Depends() const { return mInputs; }
        inline virtual const std::vector<ResourceReferenceBase*>& Provides() const { return mOutputs; }

        inline virtual void SetImageInputs(std::unordered_map<std::string_view, ManagedImage*>& dependencies) {}
        inline virtual void SetImageOutputs(std::unordered_map<std::string_view, ManagedImage*>& dependencies) {}

        inline virtual std::string_view GetName() const { return ""; }

        /// @brief After a shader recompilation has happened, the stage might want to rebuild their pipelines.
        inline virtual void OnShadersRecompiled(ShaderCompiler* shaderCompiler){};

      protected:
        std::vector<ManagedImage*> mColorAttachments;
        const VkContext*           mContext{};
        DescriptorSetHelper        mDescriptorSet;

        std::vector<ResourceReferenceBase*> mInputs;
        std::vector<ResourceReferenceBase*> mOutputs;
        VkPipeline       mPipeline       = nullptr;
        VkPipelineLayout mPipelineLayout = nullptr;

        virtual void CreateFixedSizeComponents(){};
        virtual void DestroyFixedComponents(){};
        virtual void CreateResolutionDependentComponents(){};
        virtual void DestroyResolutionDependentComponents(){};
    };
}  // namespace hsk