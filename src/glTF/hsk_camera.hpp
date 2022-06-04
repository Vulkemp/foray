#pragma once
#include "../memory/hsk_managedubo.hpp"
#include "hsk_gltf_declares.hpp"
#include "hsk_scenecomponent.hpp"
#include "../hsk_glm.hpp"
#include <tinygltf/tiny_gltf.h>
#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan.h>
#include <memory>
#include "../memory/hsk_descriptorsethelper.hpp"


namespace hsk {

    struct CameraUboBlock
    {
        glm::mat4 ProjectionMatrix = {};
        glm::mat4 ViewMatrix = {};
        glm::mat4 PreviousProjectionMatrix = {};
        glm::mat4 PreviousViewMatrix = {};
    };

    class Camera : public hsk::SceneComponent
    {
      public:
        inline explicit Camera(hsk::Scene* scene) : SceneComponent(scene), mUbo(true) {}

        void InitFromTinyGltfCamera(const tinygltf::Camera& camera);
        void InitDefault();

        void Update();

        void Cleanup();

        inline virtual ~Camera() { Cleanup(); }

        HSK_PROPERTY_GET(Ubo)
        HSK_PROPERTY_CGET(Ubo)

        inline glm::mat4& ProjectionMat()
        {
            return mUbo.GetUbo().ProjectionMatrix;
        }
        inline glm::mat4& ViewMat() { return mUbo.GetUbo().ViewMatrix; }

        std::shared_ptr<DescriptorSetHelper::DescriptorInfo> GetUboDescriptorInfo();

        void InitDefaultViewMatrix();
        void InitDefaultProjectionMatrix();

      protected:

        ManagedUbo<CameraUboBlock> mUbo;

        void InitFromTinyGltfCameraPerspective(const tinygltf::PerspectiveCamera& camera);
        void InitFromTinyGltfCameraOrthographic(const tinygltf::OrthographicCamera& camera);
    };
}  // namespace hsk