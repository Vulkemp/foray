#pragma once
#include "../core/foray_context.hpp"
#include "../core/foray_shadermodule.hpp"
#include "../scene/foray_scene.hpp"
#include "foray_rasterizedRenderStage.hpp"
#include <array>

#ifdef ENABLE_GBUFFER_BENCH
#include "../bench/foray_devicebenchmark.hpp"
#endif  // ENABLE_GBUFFER_BENCH

namespace foray::stages {
    class GBufferStage : public RasterizedRenderStage
    {
      public:
        GBufferStage() = default;

        virtual void Init(core::Context* context, scene::Scene* scene, std::string_view vertexShaderPath = "", std::string_view fragmentShaderPath = "");
        virtual void RecordFrame(VkCommandBuffer cmdBuffer, base::FrameRenderInfo& renderInfo) override;
        virtual void OnShadersRecompiled() override;

        virtual void OnResized(const VkExtent2D& extent) override;

        enum class EOutput
        {
            Position,
            Normal,
            Albedo,
            Motion,
            MaterialIdx,
            MeshInstanceIdx,
            LinearZ,
            Depth,
            MaxEnum
        };

        inline static constexpr std::string_view PositionOutputName       = "Gbuf.Position";
        inline static constexpr std::string_view NormalOutputName         = "Gbuf.Normal";
        inline static constexpr std::string_view AlbedoOutputName         = "Gbuf.Albedo";
        inline static constexpr std::string_view MotionOutputName         = "Gbuf.Motion";
        inline static constexpr std::string_view MaterialIdxOutputName    = "Gbuf.MaterialIdx";
        inline static constexpr std::string_view MeshInstanceIdOutputName = "Gbuf.MeshInstanceIdx";
        inline static constexpr std::string_view LinearZOutputName        = "Gbuf.LinearZ";
        inline static constexpr std::string_view DepthOutputName          = "Gbuf.Depth";

        core::ManagedImage* GetImageEOutput(EOutput output, bool noThrow = false);

#ifdef ENABLE_GBUFFER_BENCH
        FORAY_PROPERTY_ALLGET(Benchmark)
#endif
      protected:
        scene::Scene* mScene;

        struct PerImageInfo
        {
            EOutput            Output;
            core::ManagedImage Image;
            VkClearValue       ClearValue;
        };

        std::array<PerImageInfo, (size_t)EOutput::MaxEnum> mImageInfos;

        std::string mVertexShaderPath   = "";
        std::string mFragmentShaderPath = "";

        core::ShaderModule mVertexShaderModule;
        core::ShaderModule mFragmentShaderModule;

        virtual void CreateFixedSizeComponents() override;
        virtual void DestroyFixedComponents() override;
        virtual void CreateResolutionDependentComponents() override;
        virtual void DestroyResolutionDependentComponents() override;


        void         CreateImages();
        void         PrepareRenderpass();
        virtual void SetupDescriptors() override;
        virtual void CreateDescriptorSets() override;
        virtual void CreatePipelineLayout() override;
        void         CreatePipeline();

#ifdef ENABLE_GBUFFER_BENCH
        bench::DeviceBenchmark mBenchmark;

        inline static const char* TIMESTAMP_VERT_BEGIN = "Vertex Begin";
        inline static const char* TIMESTAMP_VERT_END   = "Vertex End";
        inline static const char* TIMESTAMP_FRAG_BEGIN = "Fragment Begin";
        inline static const char* TIMESTAMP_FRAG_END   = "Fragment End";
#endif  // ENABLE_GBUFFER_BENCH
    };
}  // namespace foray::stages