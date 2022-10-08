#pragma once
#include "../base/foray_framerenderinfo.hpp"
#include "../base/foray_shadercompiler.hpp"
#include "../core/foray_descriptorsethelper.hpp"
#include "../core/foray_managedimage.hpp"
#include "../core/foray_vkcontext.hpp"
#include "../foray_basics.hpp"

namespace foray::stages {

    class RenderStage
    {
      public:
        RenderStage(){};
        inline virtual void RecordFrame(base::FrameRenderInfo& renderInfo) {}

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
        inline std::vector<core::ManagedImage*>& GetColorAttachments() { return mColorAttachments; }
        core::ManagedImage*                      GetColorAttachmentByName(const std::string_view name, bool noThrow = false);

        inline virtual std::string_view GetName() const { return ""; }

        /// @brief After a shader recompilation has happened, the stage might want to rebuild their pipelines.
        inline virtual void OnShadersRecompiled(base::ShaderCompiler* shaderCompiler){};

      protected:
        std::vector<core::ManagedImage*> mColorAttachments;
        const core::VkContext*           mContext{};
        core::DescriptorSetHelper        mDescriptorSet;

        VkPipeline       mPipeline       = nullptr;
        VkPipelineLayout mPipelineLayout = nullptr;

        virtual void CreateFixedSizeComponents(){};
        virtual void DestroyFixedComponents(){};
        virtual void CreateResolutionDependentComponents(){};
        virtual void DestroyResolutionDependentComponents(){};
    };
}  // namespace foray::stages