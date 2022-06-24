#pragma once
#include "../base/hsk_vkcontext.hpp"
#include "../scenegraph/hsk_scene.hpp"
#include "hsk_rasterizedRenderStage.hpp"

namespace hsk {
    class GBufferStage : public RasterizedRenderStage
    {
      public:
        GBufferStage() = default;

        virtual void Init(const VkContext* context, Scene* scene);
        virtual void RecordFrame(FrameRenderInfo& renderInfo) override;

        inline static constexpr std::string_view WorldspacePosition = "Position";
        inline static constexpr std::string_view WorldspaceNormal   = "Normal";
        inline static constexpr std::string_view Albedo             = "Albedo";
        inline static constexpr std::string_view MotionVector       = "Motion";
        inline static constexpr std::string_view MeshInstanceIndex  = "MeshId";
        inline static constexpr std::string_view MaterialIndex      = "MaterialId";

      protected:
        Scene* mScene;
        std::vector<VkClearValue> mClearValues;
        std::vector<std::unique_ptr<ManagedImage>> mGBufferImages;

        virtual void CreateFixedSizeComponents() override;
        virtual void DestroyFixedComponents() override;
        virtual void CreateResolutionDependentComponents() override;
        virtual void DestroyResolutionDependentComponents() override;
        

        void PrepareAttachments();
        void PrepareRenderpass();
        void SetupDescriptors();
        void BuildCommandBuffer(){};
        void PreparePipeline();
    };
}  // namespace hsk