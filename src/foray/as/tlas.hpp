#pragma once
#include "../basics.hpp"
#include "../core/managedresource.hpp"
#include "../scene/component.hpp"
#include "../scene/scene_declares.hpp"
#include "../util/dualbuffer.hpp"
#include "as_declares.hpp"
#include "blasinstance.hpp"
#include "geometrymetabuffer.hpp"
#include <map>
#include <vulkan/vulkan.h>

namespace foray::as {


    /// @brief Describes a top level accerlation structure. A tlas usually holds multiple Blas.
    /// A blas is an object/mesh instance together with its transformation in 3d space.
    class Tlas : public core::ManagedResource
    {
      public:
        using BlasInstanceMap = std::map<uint64_t, BlasInstance>;

        class Builder
        {
          public:
            /// @brief Add a BLAS instance from meshInstance component
            Builder& AddBlasInstance(scene::ncomp::MeshInstance* meshInstance);
            /// @brief Add a BLAS instance from meshInstance component
            Builder& AddBlasInstance(scene::ncomp::MeshInstance* meshInstance, uint64_t& OUT_key);
            /// @brief Add an animated BLAS instance
            Builder& AddBlasInstanceAnimated(const Blas& blas, BlasInstance::TransformUpdateFunc getUpdatedGlobalTransformFunc);
            /// @brief Add an animated BLAS instance
            Builder& AddBlasInstanceAnimated(const Blas& blas, BlasInstance::TransformUpdateFunc getUpdatedGlobalTransformFunc, uint64_t& OUT_key);
            /// @brief Add a static BLAS instance
            Builder& AddBlasInstanceStatic(const Blas& blas, const glm::mat4& transform);
            /// @brief Add a static BLAS instance
            Builder& AddBlasInstanceStatic(const Blas& blas, const glm::mat4& transform, uint64_t& OUT_key);

            /// @brief Find a Blas Instance using the key obtained when adding it
            /// @return BlasInstance ptr if found, nullptr otherwise
            BlasInstance* FindBlasInstance(uint64_t key);
            /// @brief Attempts to remove a Blas instance with a matching key
            Builder& RemoveBlasInstance(uint64_t key);

            FORAY_PROPERTY_R(AnimatedBlasInstances)
            FORAY_PROPERTY_R(StaticBlasInstances)
            FORAY_PROPERTY_V(AccelerationStructure)
          protected:
            BlasInstanceMap            mAnimatedBlasInstances;
            BlasInstanceMap            mStaticBlasInstances;
            uint64_t                   mNextKey   = 0;
            vk::AccelerationStructureKHR mAccelerationStructure = nullptr;
        };

        Tlas(core::Context* context, const Builder& builder);

        virtual ~Tlas();

        inline virtual std::string_view GetTypeName() const override { return "Top-Level AS"; }

        /// @brief Updates transforms only. TLAS rebuild is performed and synchronized on GPU only. TLAS must by non-Dirty!
        virtual void CmdUpdate(VkCommandBuffer cmdBuffer, uint32_t frameIndex);

        FORAY_GETTER_V(AccelerationStructure)
        FORAY_GETTER_CR(TlasMemory)
        FORAY_GETTER_V(TlasAddress)
        FORAY_GETTER_MEM(MetaBuffer)

        operator vk::AccelerationStructureKHR() { return mAccelerationStructure; }

        /// @brief Get A BLAS instance by id
        const BlasInstance* GetBlasInstance(uint64_t id) const;

        FORAY_GETTER_CR(AnimatedBlasInstances)
        FORAY_GETTER_CR(StaticBlasInstances)

      protected:
        core::Context*                   mContext               = nullptr;
        vk::AccelerationStructureKHR       mAccelerationStructure = nullptr;
        Local<core::ManagedBuffer>       mTlasMemory;
        Local<util::DualBuffer>          mInstanceBuffer;
        Local<core::ManagedBuffer>       mScratchBuffer;
        vk::DeviceAddress                  mTlasAddress = 0;
        std::map<uint64_t, BlasInstance> mAnimatedBlasInstances;
        std::map<uint64_t, BlasInstance> mStaticBlasInstances;
        Local<GeometryMetaBuffer>        mMetaBuffer;
    };
}  // namespace foray::as