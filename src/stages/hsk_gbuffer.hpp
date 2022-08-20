#pragma once
#include "../base/hsk_vkcontext.hpp"
#include "../scenegraph/hsk_scene.hpp"
#include "hsk_rasterizedRenderStage.hpp"

#ifdef ENABLE_GBUFFER_BENCH
#include "../bench/hsk_devicebenchmark.hpp"
#endif  // ENABLE_GBUFFER_BENCH

namespace hsk {
    class GBufferStage : public RasterizedRenderStage
    {
      public:
        GBufferStage() = default;

        virtual void Init(const VkContext* context, Scene* scene);
        virtual void RecordFrame(FrameRenderInfo& renderInfo) override;

        inline static constexpr std::string_view WorldspacePosition = "Gbuf.Position";
        inline static constexpr std::string_view WorldspaceNormal   = "Gbuf.Normal";
        inline static constexpr std::string_view Albedo             = "Gbuf.Albedo";
        inline static constexpr std::string_view MotionVector       = "Gbuf.Motion";
        inline static constexpr std::string_view MaterialIndex      = "Gbuf.MaterialId";
        inline static constexpr std::string_view MeshInstanceIndex  = "Gbuf.MeshInstanceId";

#ifdef ENABLE_GBUFFER_BENCH
        HSK_PROPERTY_ALLGET(Benchmark)
#endif
      protected:
        Scene*                                     mScene;
        std::vector<VkClearValue>                  mClearValues;
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

#ifdef ENABLE_GBUFFER_BENCH
        DeviceBenchmark mBenchmark;

        inline static const char* TIMESTAMP_VERT_BEGIN = "Vertex Begin";
        inline static const char* TIMESTAMP_VERT_END   = "Vertex End";
        inline static const char* TIMESTAMP_FRAG_BEGIN = "Fragment Begin";
        inline static const char* TIMESTAMP_FRAG_END   = "Fragment End";
#endif  // ENABLE_GBUFFER_BENCH
    };
}  // namespace hsk