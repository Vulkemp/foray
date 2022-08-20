#pragma once
#include "../base/hsk_vkcontext.hpp"
#include "../scenegraph/hsk_scene.hpp"
#include "hsk_rasterizedRenderStage.hpp"
#include "../memory/hsk_managedbuffer.hpp"
#include "../memory/hsk_managedimage.hpp"

// heavily inspired by https://github.com/KhronosGroup/Vulkan-Samples/blob/master/samples/extensions/raytracing_basic/raytracing_basic.cpp
namespace hsk {
    class RaytracingStage : public RenderStage
    {
      public:
        RaytracingStage() = default;

        virtual void Init(const VkContext* context, Scene* scene);
        virtual void RecordFrame(FrameRenderInfo& renderInfo) override;
        virtual void OnShadersRecompiled(ShaderCompiler* shaderCompiler) override;

        inline static constexpr std::string_view RaytracingRenderTargetName = "RaytraycingRenderTarget";

        virtual void OnResized(const VkExtent2D& extent) override;
      protected:
        Scene*                                     mScene;
        std::vector<VkClearValue>                  mClearValues;
        std::vector<std::unique_ptr<ManagedImage>> mGBufferImages;

        virtual void CreateFixedSizeComponents();
        virtual void DestroyFixedComponents();
        virtual void CreateResolutionDependentComponents();
        virtual void DestroyResolutionDependentComponents();

        void PrepareAttachments();
        void PrepareRenderpass();
        void SetupDescriptors();
        void UpdateDescriptors();
        void CreatePipelineLayout();
        void CreateShaderBindingTables();
        void CreateRaytraycingPipeline();

        std::shared_ptr<DescriptorSetHelper::DescriptorInfo> GetAccelerationStructureDescriptorInfo(bool rebuild = false);
        std::shared_ptr<DescriptorSetHelper::DescriptorInfo> GetRenderTargetDescriptorInfo(bool rebuild = false);

        /// @brief Storage image that the ray generation shader will be writing to.
        ManagedImage mRaytracingRenderTarget;

        VkPipeline                 mPipeline{};
        VkPipelineLayout           mPipelineLayout{};
        VkFramebuffer              mFrameBuffer   = nullptr;
        VkPipelineCache            mPipelineCache = nullptr;
        VkRenderPass               mRenderpass    = nullptr;

        std::unique_ptr<ManagedBuffer>                    mRaygenShaderBindingTable;
        std::unique_ptr<ManagedBuffer>                    mMissShaderBindingTable;
        std::unique_ptr<ManagedBuffer>                    mHitShaderBindingTable;
        std::vector<VkRayTracingShaderGroupCreateInfoKHR> mShaderGroups{};

        VkPhysicalDeviceRayTracingPipelinePropertiesKHR  mRayTracingPipelineProperties{};
        VkPhysicalDeviceAccelerationStructureFeaturesKHR mAccelerationStructureFeatures{};

        VkWriteDescriptorSetAccelerationStructureKHR         mDescriptorAccelerationStructureInfo{};
        std::shared_ptr<DescriptorSetHelper::DescriptorInfo> mAcclerationStructureDescriptorInfo;

        std::vector<VkDescriptorImageInfo>                   mRenderTargetDescriptorImageInfos;
        std::shared_ptr<DescriptorSetHelper::DescriptorInfo> mRenderTargetDescriptorInfo{};
        void                                                 UpdateRenderTargetDescriptorBufferInfos();

        inline uint32_t aligned_size(uint32_t value, uint32_t alignment) { return (value + alignment - 1) & ~(alignment - 1); }
    };
}  // namespace hsk