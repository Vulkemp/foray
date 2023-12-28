#pragma once
#include "../bench/hostbenchmark.hpp"
#include "../scene/animation.hpp"
#include "../scene/geo.hpp"
#include "../scene/scene.hpp"
#include "../scene/scene_declares.hpp"
#include "../osi/path.hpp"
#include <map>
#include <set>
#include <tinygltf/tiny_gltf.h>

namespace foray::gltf {

    /// @brief Options for converting glTF models to the integrated scene graph (foray::scene::Scene)
    struct ModelConverterOptions
    {
        using SceneSelectFunctionPointer = std::function<int32_t(tinygltf::Model&)>;

        /// @brief A model can define multiple scenes. Index 0 is selected automatically, set this function pointer to override this behavior
        SceneSelectFunctionPointer SceneSelect = nullptr;
        /// @brief Flip the model in the Y direction on loading (experimental)
        /// @details
        /// If true:
        ///  - Y translation is inverted
        ///  - Y Vertex positions and normals are inverted
        ///  - Vertex order is flipped
        /// 
        /// Problems:
        ///  - Rotations are likely to break
        ///
        /// It is recommended to instead flip the viewport at some point later in the pipeline (for example when blitting to swapchain)
        bool FlipY = false;
    };

    /// @brief Type which reads glTF files and merges a scene of the file into the scene graph
    class ModelConverter : public NoMoveDefaults
    {
      public:
        explicit ModelConverter(scene::Scene* scene);

        /// @brief Loads glTF model and integrates it into the scene set in constructor
        /// @param utf8Path Relative or absolute path to a .gltf or .glb file
        /// @param context Set this to override use of the scene's context
        /// @param options Set this to customize default behavior
        void LoadGltfModel(osi::Utf8Path utf8Path, core::Context* context = nullptr, const ModelConverterOptions& options = ModelConverterOptions());

        FORAY_GETTER_V(Scene)

        FORAY_GETTER_MR(Benchmark)

      protected:
        core::Context* mContext = nullptr;

        // Tinygltf stuff

        tinygltf::Model  mGltfModel = {};
        tinygltf::Scene* mGltfScene = nullptr;

        // Temporary structures

        ModelConverterOptions mOptions;

        osi::Utf8Path mUtf8Dir;

        std::vector<scene::Vertex>* mVertexBuffer = nullptr;
        std::vector<uint32_t>*      mIndexBuffer  = nullptr;

        /// @brief Variables which determine how to map gltf-model indices to scene indices/pointers
        struct IndexBindings
        {
            /// @brief Offset for translating gltfModel node index -> scene node buffer index
            std::vector<scene::Node*> Nodes;
            /// @brief Offset for translating gltfModel material index -> scene material buffer index
            int32_t MaterialBufferOffset;
            /// @brief Vector mapping gltfModel mesh index to Mesh*
            std::vector<scene::Mesh*> Meshes;
            /// @brief Vector mapping gltfModel texture index to Image*
            int32_t TextureBufferOffset;
            size_t  IndexBufferStart;
        } mIndexBindings = {};

        int32_t mNextMeshInstanceIndex = 0;

        // Result structures

        scene::Scene* mScene = nullptr;

        scene::gcomp::MaterialManager& mMaterialBuffer;
        scene::gcomp::GeometryStore&  mGeo;
        scene::gcomp::TextureManager&   mTextures;
        bench::HostBenchmark   mBenchmark;

        static void sTranslateSampler(const tinygltf::Sampler& tinygltfSampler, vk::SamplerCreateInfo& outsamplerCI, bool& generateMipMaps);

        void RecursivelyTranslateNodes(int32_t currentIndex, scene::Node* parent = nullptr);

        // void LoadMesh

        void InitTransformFromGltf(scene::ncomp::Transform*          transform,
                                   const std::vector<double>& matrix,
                                   const std::vector<double>& translation,
                                   const std::vector<double>& rotation,
                                   const std::vector<double>& scale);

        void BuildGeometryBuffer();
        void PushGltfMeshToBuffers(const tinygltf::Mesh& mesh, std::vector<scene::Primitive>& outprimitives);

        void LoadTextures();
        void LoadMaterials();
        void LoadAnimations();
        void TranslateAnimationSampler(scene::Animation&                                                 animation,
                                       const tinygltf::Animation&                                        gltfAnimation,
                                       int32_t                                                           samplerIndex,
                                       const std::map<std::string_view, scene::EAnimationInterpolation>& interpolationMap,
                                       std::map<int, int>&                                               samplerIndexMap);

        void TranslateLight(scene::ncomp::PunctualLight* component, const tinygltf::Light& light);

        void TranslateCamera(scene::ncomp::Camera* component, const tinygltf::Camera& camera);

        void DetectAnimatedNodes();

        void InitialUpdate();

        void Reset();
    };
}  // namespace foray::gltf