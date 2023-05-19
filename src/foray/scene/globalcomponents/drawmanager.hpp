#pragma once
#include "../../util/dualbuffer.hpp"
#include "../component.hpp"

namespace foray::scene::gcomp {

    /// @brief Represents instanced draw operation of a single mesh
    struct DrawOp
    {
      public:
        uint64_t                          Order           = 0;
        Mesh*                             Target          = nullptr;
        std::vector<ncomp::MeshInstance*> Instances       = {};
        uint32_t                          TransformOffset = 0;
    };

    /// @brief Manages a collection of mesh instances, current and previous model matrices
    class DrawDirector : public GlobalComponent, public Component::UpdateCallback, public Component::DrawCallback
    {
      public:
        inline DrawDirector() : Component::UpdateCallback(ORDER_TRANSFORM) {}

        /// @brief Collects mesh instances and rebuilds transform buffers
        void InitOrUpdate();

        /// @brief Updates and uploads transform buffers
        virtual void Update(SceneUpdateInfo&) override;
        /// @brief Draws the scene using the currently bound pipeline and renderpass. Vertex and Index buffers must be bound
        virtual void Draw(SceneDrawInfo&) override;

        FORAY_GETTER_MEM(CurrentTransformBuffer)
        FORAY_GETTER_MEM(PreviousTransformBuffer)

        inline VkDescriptorBufferInfo GetCurrentTransformsDescriptorInfo() const { return mCurrentTransformBuffer->GetDeviceBuffer().GetVkDescriptorBufferInfo(); }
        inline VkDescriptorBufferInfo GetPreviousTransformsDescriptorInfo() const { return mPreviousTransformBuffer->GetVkDescriptorBufferInfo(); }
        inline VkBuffer               GetCurrentTransformsVkBuffer() const { return mCurrentTransformBuffer->GetDeviceBuffer().GetBuffer(); }
        inline VkBuffer               GetPreviousTransformsVkBuffer() const { return mPreviousTransformBuffer->GetBuffer(); }

        FORAY_GETTER_V(TotalCount)

      protected:
        Local<util::DualBuffer>    mCurrentTransformBuffer;
        Local<core::ManagedBuffer> mPreviousTransformBuffer;

        void CreateBuffers(size_t transformCount);
        void DestroyBuffers();

        /// @brief Draw Op structs store draw operation
        std::vector<DrawOp> mDrawOps    = {};
        bool                mFirstSetup = true;
        GeometryStore*      mGeo        = nullptr;
        uint32_t            mTotalCount = 0;
    };
}  // namespace foray::scene::gcomp