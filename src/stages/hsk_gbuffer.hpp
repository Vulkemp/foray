#pragma once
#include "../base/hsk_vkcontext.hpp"
#include "../glTF/hsk_scene.hpp"
#include "hsk_rasterizedRenderStage.hpp"

namespace hsk {
    class GBufferStage : public RasterizedRenderStage
    {
      public:
        GBufferStage() = default;
        virtual ~GBufferStage() { Destroy(); }

        virtual void Init(const VkContext* context, Scene* scene);
        virtual void RecordFrame(FrameRenderInfo& renderInfo) override;
        virtual void Destroy();

        virtual void OnResized(VkExtent2D& extent);

        inline static constexpr std::string_view WorldspacePosition = "Position";
        inline static constexpr std::string_view WorldspaceNormal   = "Normal";
        inline static constexpr std::string_view Albedo             = "Albedo";
        inline static constexpr std::string_view MotionVector       = "Motion";
        inline static constexpr std::string_view MeshInstanceIndex  = "MeshId";
        inline static constexpr std::string_view MaterialIndex      = "MaterialId";

      protected:
        Scene* mScene;
        std::vector<VkClearValue> mClearValues;

        virtual void CreateFixedSizeComponents();
        virtual void CreateResolutionDependentComponents();
        virtual void DestroyResolutionDependentComponents();

        void PrepareAttachments();
        void PrepareRenderpass();
        void SetupDescriptors();
        void BuildCommandBuffer(){};
        void PreparePipeline();
    };
}  // namespace hsk