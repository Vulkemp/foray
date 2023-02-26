#pragma once
#include "../core/foray_shadermodule.hpp"
#include "../scene/foray_scene_declares.hpp"
#include "foray_rasterizedRenderStage.hpp"

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
            VkFormat ImageFormat = VkFormat::VK_FORMAT_UNDEFINED;
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

        /// @brief Enable a builtin feature (such as ALPHATEST) regardless of outputs generated
        ConfigurableRasterStage& EnableBuiltInFeature(BuiltInFeaturesFlagBits feature);

        /// @brief Add an Output to the GBuffer
        /// @remarks MUST be called before Build(), ONLY MAX CGBuffer::MAX_OUTPUT_COUNT may be set!
        /// @param name Identifier (access the generated image via GetImageOutput(name))
        /// @param recipe Information for layout and type of data generated and calculation
        ConfigurableRasterStage& AddOutput(std::string_view name, const OutputRecipe& recipe);
        /// @brief Readonly access to an output recipe
        const OutputRecipe& GetOutputRecipe(std::string_view name) const;

        /// @brief Builds the GBuffer. Make sure to add all outputs before!
        virtual void Build(core::Context* context, scene::Scene* scene, std::string_view name = "ConfigurableRasterStage");

        virtual void RecordFrame(VkCommandBuffer cmdBuffer, base::FrameRenderInfo& renderInfo) override;

        virtual void Resize(const VkExtent2D& extent) override;

        virtual void Destroy() override;

        virtual ~ConfigurableRasterStage() { Destroy(); }

        /// @brief Gets the depth image
        core::ManagedImage* GetDepthImage();

        FORAY_PROPERTY_V(FlipY)

        /// @brief Get maximum attachment count that a device supports
        static uint32_t GetDeviceMaxAttachmentCount(vkb::Device* device);
        /// @brief Get maximum attachment count that a device supports
        static uint32_t GetDeviceMaxAttachmentCount(vkb::PhysicalDevice* device);

      protected:
        struct Output
        {
            std::string        Name;
            core::ManagedImage Image;
            OutputRecipe       Recipe;

            inline Output(std::string_view name, const OutputRecipe& recipe) : Name(name), Recipe(recipe) {}
            VkAttachmentDescription GetAttachmentDescr() const;
        };
        using OutputMap  = std::unordered_map<std::string, std::unique_ptr<Output>>;
        using OutputList = std::vector<Output*>;

        OutputMap          mOutputMap;
        OutputList         mOutputList;
        core::ManagedImage mDepthImage;
        scene::Scene*      mScene = nullptr;

        uint32_t mBuiltInFeaturesFlagsGlobal = 0;
        uint32_t mInterfaceFlagsGlobal       = 0;

        core::ShaderModule mVertexShaderModule;
        core::ShaderModule mFragmentShaderModule;

        std::string mDepthOutputName = "";
        std::string mName            = "";

        bool mFlipY = false;

        static std::string ToString(FragmentOutputType type);
        static std::string ToString(BuiltInFeaturesFlagBits feature);
        static std::string ToString(FragmentInputFlagBits input);

        virtual void CheckDeviceColorAttachmentCount();
        virtual void CreateOutputs(const VkExtent2D& size);
        virtual void CreateRenderPass();
        virtual void CreateFrameBuffer();
        virtual void SetupDescriptors() override;
        virtual void CreateDescriptorSets() override;
        virtual void CreatePipelineLayout() override;
        virtual void ConfigureAndCompileShaders();
        virtual void CreatePipeline();
    };
}  // namespace foray::stages