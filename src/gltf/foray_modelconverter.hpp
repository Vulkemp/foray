#pragma once
#include "../bench/foray_hostbenchmark.hpp"
#include "../scene/foray_animation.hpp"
#include "../scene/foray_geo.hpp"
#include "../scene/foray_scene.hpp"
#include "../scene/foray_scene_declares.hpp"
#include <map>
#include <set>
#include <tinygltf/tiny_gltf.h>

namespace foray::gltf {
    class ModelConverter : public NoMoveDefaults
    {
      public:
        explicit ModelConverter(scene::Scene* scene);

        void LoadGltfModel(std::string utf8Path, const core::VkContext* context = nullptr, std::function<int32_t(tinygltf::Model)> sceneSelect = nullptr);

        FORAY_PROPERTY_ALL(Scene)

        static void sTranslateSampler(const tinygltf::Sampler& tinygltfSampler, VkSamplerCreateInfo& outsamplerCI);

        FORAY_PROPERTY_CGET(Benchmark)

      protected:
        const core::VkContext* mContext = nullptr;

        // Tinygltf stuff

        tinygltf::Model  mGltfModel = {};
        tinygltf::Scene* mGltfScene = nullptr;

        // Temporary structures

        std::string mUtf8Dir;

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
            /// @brief Vector mapping gltfModel texture index to ManagedImage*
            int32_t TextureBufferOffset;
            size_t  IndexBufferStart;
        } mIndexBindings = {};

        int32_t mNextMeshInstanceIndex = 0;

        // Result structures

        scene::Scene* mScene = nullptr;

        scene::MaterialBuffer& mMaterialBuffer;
        scene::GeometryStore&  mGeo;
        scene::TextureStore&   mTextures;
        bench::HostBenchmark   mBenchmark;

        void RecursivelyTranslateNodes(int32_t currentIndex, scene::Node* parent = nullptr);

        // void LoadMesh

        void InitTransformFromGltf(scene::Transform*          transform,
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

        void DetectAnimatedNodes();

        void InitialUpdate();

        void Reset();
    };
}  // namespace foray::gltf