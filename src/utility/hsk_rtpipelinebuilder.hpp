#pragma once
#include "../hsk_basics.hpp"
#include "../memory/hsk_managedbuffer.hpp"
#include "hsk_shadermodule.hpp"
#include <map>
#include <unordered_map>
#include <unordered_set>

namespace hsk {

    class MemoryArea
    {
      public:
        void*  Memory;
        size_t Size;

        MemoryArea();
        MemoryArea(void* mem, size_t size);
    };

    enum class RtShaderType
    {
        Undefined,
        Raygen,
        ClosestHit,
        Anyhit,
        Intersect,
        Miss,
        Callable
    };

    enum class RtShaderGroupType
    {
        Undefined,
        Raygen,
        Miss,
        Callable,
        Intersect,
    };

    class RtShaderEnumConversions
    {
      public:
        static VkShaderStageFlagBits ToStage(RtShaderType shaderType);
        static RtShaderGroupType     ToGroupType(RtShaderType shaderType);
        static RtShaderGroupType     ToGroupType(VkShaderStageFlagBits stage);
        static RtShaderType          ToType(VkShaderStageFlagBits stage);
    };

    using ShaderHandle  = uint64_t;  // According to vulkan.gpuinfo.org, as of 2022-09, all GPUs use 32 bit values for shader handles. This should be ample for future.
    using SbtBindId     = int32_t;
    using ShaderGroupId = int32_t;

    class RtShaderCollection;

    /// @brief A shader binding table is essentially a list of shader references, with some optional custom data (indices etc.) per shader reference
    class ShaderBindingTable
    {
      public:
        /// @brief Add a shader without custom data
        SbtBindId AddShader(ShaderModule* shader, RtShaderType type, ShaderGroupId groupId = 0);
        /// @brief Add shader with custom data
        /// @param data pointer to a memory area of mShaderDataSize size. Use SetShaderDataSize(...) before adding shaders!
        SbtBindId AddShader(ShaderModule* shader, RtShaderType type, const void* data, ShaderGroupId groupId = 0);

        /// @brief Creates / updates the buffer
        void Build(const VkContext* context, const VkPhysicalDeviceRayTracingPipelinePropertiesKHR& pipelineProperties, const std::vector<ShaderHandle>& handles);

        HSK_PROPERTY_ALLGET(ShaderDataSize)
        HSK_PROPERTY_ALLGET(DeviceAddressRegion)
        HSK_PROPERTY_ALLGET(Buffer)
        HSK_PROPERTY_ALLGET(Shaders)

        void* ShaderDataAt(SbtBindId index);
        void* ShaderDataAt(SbtBindId index) const;

        template <typename TData>
        TData& ShaderDataAt(SbtBindId index)
        {
            return *(reinterpret_cast<TData*>(ShaderDataAt(index)));
        }
        template <typename TData>
        TData& ShaderDataAt(SbtBindId index) const
        {
            return *(reinterpret_cast<TData*>(ShaderDataAt(index)));
        }

        void WriteToShaderStageCiVector(std::vector<VkPipelineShaderStageCreateInfo>& out) const;
        void WriteToShaderCollection(RtShaderCollection& collection) const;
        void GetShaderGroupIds(std::unordered_set<uint32_t>& groupIds) const;
        void GetGroupIdIndex(uint32_t groupId, int32_t& first) const;
        void GetGroupIdIndices(uint32_t groupId, int32_t& first, int32_t& count) const;

        virtual ~ShaderBindingTable();

        struct ShaderReference
        {
            ShaderModule* Module;
            RtShaderType  Type;
            SbtBindId     BindingId = {};
            ShaderGroupId GroupId   = {};
        };

      protected:
        ManagedBuffer                   mBuffer;
        VkStridedDeviceAddressRegionKHR mDeviceAddressRegion{};

        VkDeviceSize mShaderDataSize{};

        std::vector<ShaderReference> mShaders;
        std::vector<uint8_t>         mShaderData{};
    };

    class RtShaderCollection
    {
      public:
        struct Entry
        {
            ShaderModule* Module       = nullptr;
            RtShaderType  Type         = RtShaderType::Undefined;
            uint32_t      StageCiIndex = VK_SHADER_UNUSED_KHR;
        };

        void     Add(ShaderModule* module, RtShaderType type);
        void     Clear();
        uint32_t IndexOf(ShaderModule* module) const;

        void BuildShaderStageCiVector();

        HSK_PROPERTY_CGET(ShaderStageCis)

      protected:
        std::unordered_map<ShaderModule*, Entry>     mEntries;
        std::vector<VkPipelineShaderStageCreateInfo> mShaderStageCis;
    };

    class RtPipeline
    {
      public:
        HSK_PROPERTY_ALLGET(RaygenSbt)
        HSK_PROPERTY_ALLGET(IntersectsSbt)
        HSK_PROPERTY_ALLGET(MissSbt)
        HSK_PROPERTY_ALLGET(CallablesSbt)
        HSK_PROPERTY_ALLGET(Pipeline)
        HSK_PROPERTY_ALL(PipelineLayout)

        uint32_t AddShaderGroupRaygen(ShaderModule* module);
        uint32_t AddShaderGroupCallable(ShaderModule* module);
        uint32_t AddShaderGroupMiss(ShaderModule* module);
        uint32_t AddShaderGroupIntersect(ShaderModule* closestHit, ShaderModule* anyHit, ShaderModule* intersect);

        void Build(const VkContext* context);

        struct ShaderGroup
        {
            ShaderGroupId     Id         = {};
            RtShaderGroupType Type       = RtShaderGroupType::Undefined;
            ShaderModule*     General    = nullptr;
            ShaderModule*     ClosestHit = nullptr;
            ShaderModule*     AnyHit     = nullptr;
            ShaderModule*     Intersect  = nullptr;

            VkRayTracingShaderGroupCreateInfoKHR BuildShaderGroupCi(const RtShaderCollection& shaderCollection) const;
        };

      protected:
        ShaderBindingTable mRaygenSbt;
        ShaderBindingTable mIntersectsSbt;
        ShaderBindingTable mMissSbt;
        ShaderBindingTable mCallablesSbt;

        std::vector<ShaderGroup> mShaderGroups;

        RtShaderCollection mShaderCollection;

        VkPipelineLayout mPipelineLayout;
        VkPipeline       mPipeline;
    };
}  // namespace hsk