#pragma once
#include "../scenegraph/hsk_geo.hpp"
#include "../scenegraph/hsk_scene.hpp"
#include "../scenegraph/hsk_scenegraph_declares.hpp"
#include "../scenegraph/hsk_animation.hpp"
#include <map>
#include <set>
#include <tinygltf/tiny_gltf.h>

namespace hsk {
    class ModelConverter : public NoMoveDefaults
    {
      public:
        explicit ModelConverter(Scene* scene);

        void LoadGltfModel(std::string utf8Path, const VkContext* context = nullptr, std::function<int32_t(tinygltf::Model)> sceneSelect = nullptr);

        HSK_PROPERTY_ALL(Scene)

      protected:
        const VkContext* mContext = nullptr;

        // Tinygltf stuff

        tinygltf::Model  mGltfModel = {};
        tinygltf::Scene* mGltfScene = nullptr;

        // Temporary structures

        std::vector<Vertex>   mVertexBuffer = {};
        std::vector<uint32_t> mIndexBuffer  = {};

        /// @brief Variables which determine how to map gltf-model indices to scene indices/pointers
        struct IndexBindings
        {
            /// @brief Offset for translating gltfModel node index -> scene node buffer index
            std::vector<Node*> Nodes;
            /// @brief Offset for translating gltfModel material index -> scene material buffer index
            int32_t MaterialBufferOffset;
            /// @brief Vector mapping gltfModel mesh index to Mesh*
            std::vector<Mesh*> Meshes;
            /// @brief Vector mapping gltfModel texture index to ManagedImage*
            int32_t TextureBufferOffset;
        } mIndexBindings = {};


        int32_t mNextMeshInstanceIndex = 0;

        // Result structures

        Scene* mScene = nullptr;

        MaterialBuffer& mMaterialBuffer;
        GeometryStore&  mGeo;
        TextureStore&   mTextures;

        void RecursivelyTranslateNodes(int32_t currentIndex, Node* parent = nullptr);

        // void LoadMesh

        void InitTransformFromGltf(
            Transform* transform, const std::vector<double>& matrix, const std::vector<double>& translation, const std::vector<double>& rotation, const std::vector<double>& scale);

        void BuildGeometryBuffer();
        void PushGltfMeshToBuffers(const tinygltf::Mesh& mesh, std::vector<Primitive>& outprimitives);

        void LoadTextures();
        void TranslateSampler(const tinygltf::Sampler& tinygltfSampler, VkSamplerCreateInfo& outsamplerCI);
        void LoadMaterials();
        void LoadAnimations();
        void TranslateAnimationSampler(Animation&                                                 animation,
                                       const tinygltf::Animation&                                 gltfAnimation,
                                       int32_t                                                    samplerIndex,
                                       const std::map<std::string_view, EAnimationInterpolation>& interpolationMap,
                                       std::map<int, int>&                                        samplerIndexMap);

        void InitialUpdate();

        void Reset();
    };
}  // namespace hsk