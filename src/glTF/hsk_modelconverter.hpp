#pragma once
#include "../scenegraph/hsk_scene.hpp"
#include "../scenegraph/hsk_geo.hpp"
#include <tinygltf/tiny_gltf.h>

namespace hsk{
    class ModelConverter : public NoMoveDefaults
    {
        public:
            std::unique_ptr<NScene> LoadGltfModel(const VkContext* context, std::string utf8Path);
        protected:
            tinygltf::Model mGltfModel;
            tinygltf::Scene* mGltfScene;

            std::vector<NVertex> mVertexBuffer;
            std::vector<uint32_t> mIndexBuffer;
    };
}