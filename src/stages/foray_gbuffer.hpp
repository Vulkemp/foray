#pragma once
#include "../bench/foray_bench_declares.hpp"
#include "../core/foray_context.hpp"
#include "../core/foray_shadermodule.hpp"
#include "../scene/foray_scene.hpp"
#include "foray_rasterizedRenderStage.hpp"
#include <array>

namespace foray::stages {
    /// @brief Utilizes rasterization to render a GBuffer output
    /// @details
    /// # Outputs
    ///  * Position         rgba32f     Worldspace positions of fragments 
    ///  * Normal           rgba32f     Worldspace normals
    ///  * Albedo           rgba32f     Fragment Albedo
    ///  * Motion           rg32f       Motion vectors. Translates Current -> Previous in UV coordinates
    ///  * MaterialIdx      r32i        Material Indices. -1 indicates no material set.
    ///  * MeshInstanceIdx  r32u        Mesh Instance Index. Unique for every ncomp::MeshInstance component
    ///  * LinearZ          rg32f       Linear Depth (projected.z * projected.w), absolute linear depth Gradient
    ///  * Depth            d32f        Vulkan Depth output
    class GBufferStage : public RasterizedRenderStage
    {
      public:
        GBufferStage() = default;

        /// @param scene Scene required for transforms, vertex/index buffers, materials, textures, ...
        /// @param vertexShaderPath (optional) override with a custom vertex shader
        /// @param fragmentShaderPath (optional) override with a custom fragment shader
        /// @param benchmark (optional) enable benchmarking by passing a devicebenchmark object here
        virtual void Init(core::Context* context, scene::Scene* scene, std::string_view vertexShaderPath = "", std::string_view fragmentShaderPath = "", bench::DeviceBenchmark* benchmark = nullptr);
        
        virtual void RecordFrame(VkCommandBuffer cmdBuffer, base::FrameRenderInfo& renderInfo) override;

        virtual void Resize(const VkExtent2D& extent) override;

        virtual void Destroy() override;

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

        FORAY_GETTER_CR(Benchmark)
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

        virtual void DestroyFrameBufferAndRenderpass();


        void         CreateImages();
        void         PrepareRenderpass();
        virtual void SetupDescriptors() override;
        virtual void CreateDescriptorSets() override;
        virtual void CreatePipelineLayout() override;
        void         CreatePipeline();

        bench::DeviceBenchmark* mBenchmark = nullptr;

        inline static const char* TIMESTAMP_VERT_BEGIN = "Vertex Begin";
        inline static const char* TIMESTAMP_VERT_END   = "Vertex End";
        inline static const char* TIMESTAMP_FRAG_BEGIN = "Fragment Begin";
        inline static const char* TIMESTAMP_FRAG_END   = "Fragment End";
    };
}  // namespace foray::stages