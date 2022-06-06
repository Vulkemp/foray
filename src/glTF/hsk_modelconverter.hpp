#pragma once
#include "../scenegraph/hsk_geo.hpp"
#include "../scenegraph/hsk_scene.hpp"
#include <tinygltf/tiny_gltf.h>
#include "../scenegraph/hsk_scenegraph_declares.hpp"
#include <set>
#include <map>

namespace hsk {
    class ModelConverter : public NoMoveDefaults
    {
      public:
        explicit ModelConverter(NScene* scene);

        void LoadGltfModel(const VkContext* context, std::string utf8Path, std::function<int32_t (tinygltf::Model)> sceneSelect = nullptr);

        HSK_PROPERTY_ALL(Scene)

      protected:
        // Tinygltf stuff

        tinygltf::Model  mGltfModel;
        tinygltf::Scene* mGltfScene;

        // Temporary structures

        std::vector<NVertex>  mVertexBuffer;
        std::vector<uint32_t> mIndexBuffer;
        std::set<int32_t> mMeshesUsed;
        std::map<NMeshInstance*, int32_t> mMeshMappings;

        // Result structures

        NScene* mScene;

        NMaterialBuffer& mMaterialBuffer;
        GeometryStore& mGeo;
        TextureStore& mTextures;
        SceneTransformBuffer& mTransformBuffer;

        void RecursivelyTranslateNodes(int32_t currentIndex, NNode* parent = nullptr);

        // void LoadMesh

        void InitTransformFromGltf(NTransform* transform, const std::vector<double>& matrix, const std::vector<double>& translation, const std::vector<double>& rotation, const std::vector<double>& scale);

        void BuildGeometryBuffer();
        void PushGltfMeshToBuffers(const tinygltf::Mesh& mesh, std::vector<NPrimitive>& outprimitives);
    };
}  // namespace hsk