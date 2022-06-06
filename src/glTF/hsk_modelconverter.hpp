#pragma once
#include "../scenegraph/hsk_geo.hpp"
#include "../scenegraph/hsk_scene.hpp"
#include <tinygltf/tiny_gltf.h>
#include "../scenegraph/hsk_scenegraph_declares.hpp"

namespace hsk {
    class ModelConverter : public NoMoveDefaults
    {
      public:
        explicit ModelConverter(NScene* scene);

        void LoadGltfModel(const VkContext* context, std::string utf8Path, std::function<int32_t (tinygltf::Model)> sceneSelect = nullptr);

        HSK_PROPERTY_ALL(Scene)

      protected:
        tinygltf::Model  mGltfModel;
        tinygltf::Scene* mGltfScene;

        std::vector<NVertex>  mVertexBuffer;
        std::vector<uint32_t> mIndexBuffer;

        NScene* mScene;

        NMaterialBuffer& mMaterialBuffer;
        GeometryStore& mGeo;
        TextureStore& mTextures;
        SceneTransformBuffer& mTransformBuffer;

        int32_t mMeshInstanceIndex = 0;

        void RecursivelyTranslateNodes(int32_t currentIndex, NNode* parent = nullptr);

        void InitTransformFromGltf(NTransform* transform, const std::vector<double>& matrix, const std::vector<double>& translation, const std::vector<double>& rotation, const std::vector<double>& scale);
        
    };
}  // namespace hsk