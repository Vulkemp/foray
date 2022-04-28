#pragma once
#include "../hsk_managedubo.hpp"
#include "hsk_glTF_declares.hpp"
#include "hsk_scenecomponent.hpp"
#include <glm/glm.hpp>
#include <tinygltf/tiny_gltf.h>
#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan.h>


namespace hsk {

    struct CameraUboBlock
    {
        glm::mat4 ProjectionMat = {};
        glm::mat4 ViewMat = {};
    };

    class Camera : public hsk::SceneComponent
    {
      public:
        inline explicit Camera(hsk::Scene* scene) : SceneComponent(scene) {}

        void InitFromTinyGltfCamera(const tinygltf::Camera& camera);

        void Update();

        void Cleanup();

        inline virtual ~Camera() { Cleanup(); }

        HSK_PROPERTY_GET(Ubo)
        HSK_PROPERTY_CGET(Ubo)

        inline glm::mat4& ProjectionMat() { return mUbo->GetUbo().ProjectionMat; }
        inline glm::mat4& ViewMat() { return mUbo->GetUbo().ViewMat; }

      protected:
        std::unique_ptr<ManagedUbo<CameraUboBlock>> mUbo = nullptr;

        void InitFromTinyGltfCameraPerspective(const tinygltf::PerspectiveCamera& camera);
        void InitFromTinyGltfCameraOrthographic(const tinygltf::OrthographicCamera& camera);
    };
}  // namespace hsk