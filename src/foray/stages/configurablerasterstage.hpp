#pragma once
#include "../core/shadermodule.hpp"
#include "../mem.hpp"
#include "../scene/scene_declares.hpp"
#include "rasterizedstage.hpp"

namespace foray::stages {

    /// @brief Fully configurable rasterized stage: Customizable attachment count, output calculation and toggleable built-in features
    /// @details
    /// How to use: Add Outputs, Build, Get Outputs
    ///  - Add Outputs: See documentation of ConfigurableRasterStage::OutputRecipe and ConfigurableRasterStage::AddOutput()
    ///  - Build: See ConfigurableRasterStage::Build()
    ///  - Get Outputs: See RenderStage::GetImageOutput()
    class ConfigurableRasterStage : public RasterizedRenderStage
    {
      public:
        /// @brief Maximum amount of outputs configurable.
        /// @remark NOTE!!! All known GPUs are limited to 8 outputattachments!
        inline static constexpr uint32_t MAX_OUTPUT_COUNT = 16;

        enum class FragmentInputFlagBits : uint32_t
        {
            /// @brief vec3 WorldPos: Fragment position in world space
            WORLDPOS = 0x001,
            /// @brief vec3 WorldPosOld: Fragment position in world space (previous frame)
            WORLDPOSOLD = 0x002,
            /// @brief vec4 DevicePos: Fragment position in device coordinates
            DEVICEPOS = 0x004,
            /// @brief vec4 DevicePosOld: Fragment position in device coordinates (previous frame)
            DEVICEPOSOLD = 0x008,
            /// @brief vec3 Normal: Interpolated vertex normal
            NORMAL = 0x010,
            /// @brief vec3 Tangent: Interpolated vertex tangent
            TANGENT = 0x020,
            /// @brief vec2 UV: Interpolated texture coordinates
            UV = 0x040,
            /// @brief uint MeshInstanceId: Index of the current mesh instance
            MESHID  = 0x080,
            MAXENUM = 0x100,
        };

        enum class BuiltInFeaturesFlagBits : uint32_t
        {
            /// @brief Enables the Material Probe Feature
            /// @details Defines variables:
            ///  - `material`: MaterialBufferObject (see common/gltf.glsl)
            ///  - `probe`: MaterialProbe (see common/gltf.glsl)
            /// Enables FragmentInputs: UV
            MATERIALPROBE = 0x01,
            /// @brief Enables the Material Alpha Probe Feature
            /// @details Defines variables:
            ///  - `material`: MaterialBufferObject (see common/gltf.glsl)
            ///  - `isOpaque`: true, if fragment is opaque (alpha coverage)
            /// Enables FragmentInputs: UV
            MATERIALPROBEALPHA = 0x02,
            /// @brief Discards transparent fragments based on alpha coverage rules
            /// @remark Implicitly enables MATERIALPROBEALPHA feature
            /// @details
            /// Enables FragmentInputs: UV
            ALPHATEST = 0x04,
            /// @brief Enables NormalMapping support
            /// @remark Implicitly enables MATERIALPROBE feature
            /// @details Defines variables:
            ///  - `material`: MaterialBufferObject (see common/gltf.glsl)
            ///  - `probe`: MaterialProbe (see common/gltf.glsl)
            ///  - `normalMapped`: Interpolated vertex normal shifted according to the normal map
            /// Enables FragmentInputs: UV, NORMAL, TANGENT
            NORMALMAPPING = 0x08,
            MAXENUM       = 0x10,
        };

        /// @brief Defines which type is listed with the output location in the fragment shader
        enum class FragmentOutputType
        {
            FLOAT,
            INT,
            UINT,
            VEC2,
            VEC3,
            VEC4,
            IVEC2,
            IVEC3,
            IVEC4,
            UVEC2,
            UVEC3,
            UVEC4,
        };

        static uint32_t GetFragmentOutputComponentCount(FragmentOutputType type);

        /// @brief Defines the custom GBuffer output to generate
        struct OutputRecipe
        {
            /// @brief Flags of FragmentInputFlagBits values, indicating which fragment inputs this output requires
            uint32_t FragmentInputFlags = 0;
            /// @brief Flags of BuiltInFeaturesFlagBits values, indicating which features this output requires
            uint32_t BuiltInFeaturesFlags = 0;
            /// @brief Type of output generated. Pasted to "layout(location *) out TYPE output;"
            FragmentOutputType Type = FragmentOutputType::FLOAT;
            /// @brief Image format of the GBuffer output
            vk::Format ImageFormat = vk::Format::VK_FORMAT_UNDEFINED;
            /// @brief What value to clear the screen with
            VkClearColorValue ClearValue = {};
            /// @brief Optional calculation (pasted before result assignment)
            std::string Calculation = "";
            /// @brief Result assignment. Pasted to "output = TYPE(RESULT);"
            std::string Result = "0";

            /// @brief Add a flag to FragmentInputFlags member
            OutputRecipe& AddFragmentInput(FragmentInputFlagBits input);
            /// @brief Add a flag to BuiltInFeaturesFlags member
            OutputRecipe& EnableBuiltInFeature(BuiltInFeaturesFlagBits feature);
        };

        /// @brief Template recipes
        struct Templates
        {
            /// @brief WorldSpace Positions, rgba16f, cleared to (0,0,0,0)
            static const OutputRecipe WorldPos;
            /// @brief WorldSpace Normals, rgba16f, cleared to (0,0,0,0)
            static const OutputRecipe WorldNormal;
            /// @brief Material BaseColor, rgba16f, cleared to (0,0,0,1) (alpha always 1)
            static const OutputRecipe Albedo;
            /// @brief Material Id, r32i, cleared to (-1)
            static const OutputRecipe MaterialId;
            /// @brief Mesh Instance Id, r32i, cleared to (-1)
            static const OutputRecipe MeshInstanceId;
            /// @brief Texture UV Coordinates, rg16f, cleared to (0,0)
            static const OutputRecipe UV;
            /// @brief ScreenSpace Motion Vectors, rg16f, cleared to (0,0), projects current pixel to previous pixel in normalized UV coordinates
            static const OutputRecipe ScreenMotion;
            /// @brief WorldSpace Motion Vectors, rgba16f, cleared to (0,0,0,0), projects current world position to previous world position
            static const OutputRecipe WorldMotion;
            /// @brief Linearized depth and derivative, rg16f, cleared to (65504,0)
            static const OutputRecipe DepthAndDerivative;
        };

        class Builder
        {
          public:
            using OutputMap = std::unordered_map<std::string, OutputRecipe>;
            /// @brief Enable a builtin feature (such as ALPHATEST) regardless of outputs generated
            Builder& EnableBuiltInFeature(BuiltInFeaturesFlagBits feature);

            /// @brief Add an Output to the GBuffer
            /// @remarks MUST be called before Build(), ONLY MAX CGBuffer::MAX_OUTPUT_COUNT may be set!
            /// @param name Identifier (access the generated image via GetImageOutput(name))
            /// @param recipe Information for layout and type of data generated and calculation
            Builder& AddOutput(std::string_view name, const OutputRecipe& recipe);
            /// @brief Access to an output recipe
            OutputRecipe& GetOutputRecipe(std::string_view name);
            /// @brief Readonly access to an output recipe
            const OutputRecipe& GetOutputRecipe(std::string_view name) const;

            FORAY_GETTER_V(BuiltInFeaturesFlagsGlobal)
            FORAY_GETTER_V(InterfaceFlagsGlobal)
            FORAY_GETTER_CR(OutputMap)

          private:
            uint32_t  mBuiltInFeaturesFlagsGlobal = 0;
            uint32_t  mInterfaceFlagsGlobal       = 0;
            OutputMap mOutputMap;
        };


        /// @brief Readonly access to an output recipe
        const OutputRecipe& GetOutputRecipe(std::string_view name) const;

        /// @brief Builds the Configurable Raster Stage
        ConfigurableRasterStage(
            core::Context* context, const Builder& builder, scene::Scene* scene, RenderDomain* domain, int32_t resizeOrder = 0, std::string_view name = "ConfigurableRasterStage");

        virtual void RecordFrame(VkCommandBuffer cmdBuffer, base::FrameRenderInfo& renderInfo) override;

        virtual ~ConfigurableRasterStage();

        FORAY_GETTER_MEM(DepthImage)

        FORAY_PROPERTY_V(FlipY)

        /// @brief Get maximum attachment count that a device supports
        static uint32_t GetDeviceMaxAttachmentCount(vkb::Device* device);
        /// @brief Get maximum attachment count that a device supports
        static uint32_t GetDeviceMaxAttachmentCount(vkb::PhysicalDevice* device);

        static std::string ToString(FragmentOutputType type);
        static std::string ToString(BuiltInFeaturesFlagBits feature);
        static std::string ToString(FragmentInputFlagBits input);

      protected:
        struct Output
        {
            std::string              Name;
            core::Local_ManagedImage Image;
            OutputRecipe             Recipe;

            inline Output(std::string_view name, const OutputRecipe& recipe) : Name(name), Recipe(recipe) {}
        };
        using OutputMap  = std::unordered_map<std::string, Heap<Output>>;
        using OutputList = std::vector<Output*>;

        OutputMap                mOutputMap;
        OutputList               mOutputList;
        core::Local_ManagedImage mDepthImage;
        scene::Scene*            mScene = nullptr;

        uint32_t mBuiltInFeaturesFlagsGlobal = 0;
        uint32_t mInterfaceFlagsGlobal       = 0;

        Local<core::ShaderModule> mVertexShaderModule;
        Local<core::ShaderModule> mFragmentShaderModule;

        util::DescriptorSetSimple mDescriptorSet;

        std::string mDepthOutputName = "";
        std::string mName            = "";

        bool mFlipY = false;

        virtual void CheckDeviceColorAttachmentCount();
        virtual void CreateOutputs(const VkExtent2D& size);
        virtual void CreateRenderPass();
        virtual void SetupDescriptors();
        virtual void CreateDescriptorSets();
        virtual void CreatePipelineLayout();
        virtual void ConfigureAndCompileShaders();
        virtual void CreatePipeline();
        virtual void OnResized(VkExtent2D extent) override;
    };
}  // namespace foray::stages