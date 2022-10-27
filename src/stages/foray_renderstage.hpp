#pragma once
#include "../base/foray_framerenderinfo.hpp"
#include "../core/foray_context.hpp"
#include "../core/foray_descriptorset.hpp"
#include "../core/foray_managedimage.hpp"
#include "../foray_basics.hpp"
#include <unordered_map>

namespace foray::stages {

    class RenderStage
    {
      public:
        RenderStage(){};
        inline virtual void RecordFrame(VkCommandBuffer cmdBuffer, base::FrameRenderInfo& renderInfo) {}

        inline virtual void Destroy()
        {
            DestroyResolutionDependentComponents();
            DestroyFixedComponents();
        }


        virtual void OnResized(const VkExtent2D& extent)
        {
            DestroyResolutionDependentComponents();
            CreateResolutionDependentComponents();
        };

        /// @brief Gets a vector to all color attachments that will be included in a texture array and can be referenced in the shader pass.
        std::vector<core::ManagedImage*> GetImageOutputs();
        core::ManagedImage*              GetImageOutput(const std::string_view name, bool noThrow = false);

        /// @brief After a shader recompilation has happened, the stage might want to rebuild their pipelines.
        inline virtual void OnShadersRecompiled(){};

      protected:
        std::unordered_map<std::string, core::ManagedImage*> mImageOutputs;
        core::Context*                                       mContext{};

        virtual void CreateFixedSizeComponents(){};
        virtual void DestroyFixedComponents(){};
        virtual void CreateResolutionDependentComponents(){};
        virtual void DestroyResolutionDependentComponents(){};
    };
}  // namespace foray::stages