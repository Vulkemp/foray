#pragma once
#include "../base/hsk_vkcontext.hpp"
#include "../memory/hsk_managedbuffer.hpp"
#include "../memory/hsk_managedimage.hpp"
#include "../scenegraph/hsk_scene.hpp"
#include "../utility/hsk_shadermodule.hpp"
#include "hsk_rasterizedRenderStage.hpp"

// heavily inspired by https://github.com/KhronosGroup/Vulkan-Samples/blob/master/samples/extensions/raytracing_basic/raytracing_basic.cpp
namespace hsk {

    struct RaytracingStageShaderconfig
    {
        std::string RaygenShaderpath;
        std::string MissShaderpath;
        std::string ClosesthitShaderpath;
        std::string AnyhitShaderpath;

        static RaytracingStageShaderconfig Basic();
    };

    class RaytracingStage : public RenderStage
    {
      public:
        RaytracingStage() = default;

        virtual void Init(const VkContext* context, Scene* scene, ManagedImage* environmentMap, const RaytracingStageShaderconfig& shaderconfig);
        virtual void RecordFrame(FrameRenderInfo& renderInfo) override;
        virtual void OnShadersRecompiled(ShaderCompiler* shaderCompiler) override;

        inline static constexpr std::string_view RaytracingRenderTargetName = "RaytraycingRenderTarget";

        virtual void OnResized(const VkExtent2D& extent) override;

      protected:
        Scene*                                     mScene;
        std::vector<VkClearValue>                  mClearValues;
        std::vector<std::unique_ptr<ManagedImage>> mGBufferImages;

        virtual void CreateFixedSizeComponents() override;
        virtual void DestroyFixedComponents() override;
        virtual void CreateResolutionDependentComponents() override;
        virtual void DestroyResolutionDependentComponents() override;

        virtual void PrepareAttachments();
        virtual void SetupDescriptors();
        virtual void UpdateDescriptors();
        virtual void CreatePipelineLayout();
        virtual void CreateShaderBindingTables();
        virtual void CreateRaytraycingPipeline();

        void SetupEnvironmentMap();

        std::shared_ptr<DescriptorSetHelper::DescriptorInfo> GetAccelerationStructureDescriptorInfo(bool rebuild = false);
        std::shared_ptr<DescriptorSetHelper::DescriptorInfo> GetRenderTargetDescriptorInfo(bool rebuild = false);
        std::shared_ptr<DescriptorSetHelper::DescriptorInfo> GetEnvironmentMapDescriptorInfo(bool rebuild = false);

        /// @brief Storage image that the ray generation shader will be writing to.
        ManagedImage mRaytracingRenderTarget;

        ManagedImage*                                        mEnvironmentMap        = nullptr;
        VkSampler                                            mEnvironmentMapSampler = nullptr;
        std::vector<VkDescriptorImageInfo>                   mEnvironmentMapDescriptorImageInfos;
        std::shared_ptr<DescriptorSetHelper::DescriptorInfo> mEnvironmentMapDescriptorInfo{};
        void                                                 UpdateEnvironmentMapDescriptorInfos();

        VkPipeline       mPipeline{};
        VkPipelineLayout mPipelineLayout{};
        VkFramebuffer    mFrameBuffer   = nullptr;
        VkPipelineCache  mPipelineCache = nullptr;
        VkRenderPass     mRenderpass    = nullptr;

        struct ShaderResource
        {
            std::string   Path;
            ManagedBuffer BindingTable;
            ShaderModule  Module;
        } mRaygenShader, mMissShader, mClosesthitShader, mAnyhitShader;

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