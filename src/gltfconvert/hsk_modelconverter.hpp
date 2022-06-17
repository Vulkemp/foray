#pragma once
#include "../scenegraph/hsk_geo.hpp"
#include "../scenegraph/hsk_scene.hpp"
#include "../scenegraph/hsk_scenegraph_declares.hpp"
#include <map>
#include <set>
#include <tinygltf/tiny_gltf.h>

namespace hsk {
    class ModelConverter : public NoMoveDefaults
    {
      public:
        explicit ModelConverter(NScene* scene);

        void LoadGltfModel(std::string utf8Path, const VkContext* context = nullptr, std::function<int32_t(tinygltf::Model)> sceneSelect = nullptr);

        HSK_PROPERTY_ALL(Scene)

      protected:
        const VkContext* mContext = nullptr;

        // Tinygltf stuff

        tinygltf::Model  mGltfModel = {};
        tinygltf::Scene* mGltfScene = nullptr;

        // Temporary structures

        std::vector<NVertex>  mVertexBuffer = {};
        std::vector<uint32_t> mIndexBuffer  = {};

        struct IndexBindings
        {
            /// @brief Offset for translating gltfModel node index -> scene node buffer index
            int32_t                    NodeBufferOffset;
            /// @brief Offset for translating gltfModel material index -> scene material buffer index
            int32_t                    MaterialBufferOffset;
            /// @brief Vector mapping gltfModel mesh index to Mesh*
            std::vector<Mesh*>         Meshes;
            /// @brief Vector mapping gltfModel texture index to ManagedImage*
            std::vector<ManagedImage*> Textures;
        } mIndexBindings = {};


        int32_t mNextMeshInstanceIndex = 0;

        // Result structures

        NScene* mScene = nullptr;

        NMaterialBuffer&      mMaterialBuffer;
        GeometryStore&        mGeo;
        TextureStore&         mTextures;
        SceneTransformBuffer& mTransformBuffer;

        void RecursivelyTranslateNodes(int32_t currentIndex, NNode* parent = nullptr);

        // void LoadMesh

        void InitTransformFromGltf(NTransform*                transform,
                                   const std::vector<double>& matrix,
                                   const std::vector<double>& translation,
                                   const std::vector<double>& rotation,
                                   const std::vector<double>& scale);

        void BuildGeometryBuffer();
        void PushGltfMeshToBuffers(const tinygltf::Mesh& mesh, std::vector<NPrimitive>& outprimitives);

        void LoadTextures();
        void TranslateSampler(const tinygltf::Sampler& tinygltfSampler, VkSamplerCreateInfo& outsamplerCI);
        void InitialUpdate();

    };
}  // namespace hsk