#pragma once
#include "../core/foray_vkcontext.hpp"
#include "../scene/foray_scene.hpp"
#include "foray_rasterizedRenderStage.hpp"

#ifdef ENABLE_GBUFFER_BENCH
#include "../bench/foray_devicebenchmark.hpp"
#endif  // ENABLE_GBUFFER_BENCH

namespace foray::stages {
    class GBufferStage : public RasterizedRenderStage
    {
      public:
        GBufferStage() = default;

        virtual void Init(const core::VkContext* context, scene::Scene* scene);
        virtual void RecordFrame(base::FrameRenderInfo& renderInfo) override;
        virtual void OnShadersRecompiled() override;

        inline static constexpr std::string_view WorldspacePosition = "Gbuf.Position";
        inline static constexpr std::string_view WorldspaceNormal   = "Gbuf.Normal";
        inline static constexpr std::string_view Albedo             = "Gbuf.Albedo";
        inline static constexpr std::string_view MotionVector       = "Gbuf.Motion";
        inline static constexpr std::string_view MaterialIndex      = "Gbuf.MaterialId";
        inline static constexpr std::string_view MeshInstanceIndex  = "Gbuf.MeshInstanceId";
        core::ManagedImage*                      GetDepthBuffer() { return &mDepthAttachment; }

#ifdef ENABLE_GBUFFER_BENCH
        FORAY_PROPERTY_ALLGET(Benchmark)
#endif
      protected:
        scene::Scene*                                    mScene;
        std::vector<VkClearValue>                        mClearValues;
        core::ManagedImage                               mDepthAttachment;
        std::vector<std::unique_ptr<core::ManagedImage>> mGBufferImages;
        std::string                                      mVertexShaderPath   = "../foray/src/shaders/gbuffer/gbuffer_stage.vert";
        std::string                                      mFragmentShaderPath = "../foray/src/shaders/gbuffer/gbuffer_stage.frag";

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
        bench::DeviceBenchmark mBenchmark;

        inline static const char* TIMESTAMP_VERT_BEGIN = "Vertex Begin";
        inline static const char* TIMESTAMP_VERT_END   = "Vertex End";
        inline static const char* TIMESTAMP_FRAG_BEGIN = "Fragment Begin";
        inline static const char* TIMESTAMP_FRAG_END   = "Fragment End";
#endif  // ENABLE_GBUFFER_BENCH
    };
}  // namespace foray::stages