#pragma once
#include "../hsk_basics.hpp"
#include "../memory/hsk_managedbuffer.hpp"
#include "hsk_shadermodule.hpp"
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
        Intersect,
        Miss,
        Callable
    };

    class RtShaderEnumConversions
    {
      public:
        static VkShaderStageFlagBits ToStage(RtShaderType shaderType);
        static RtShaderGroupType     ToGroupType(RtShaderType shaderType);
        static RtShaderGroupType     ToGroupType(VkShaderStageFlagBits stage);
        static RtShaderType          ToType(VkShaderStageFlagBits stage);
    };

    using ShaderHandle = uint64_t;  // According to vulkan.gpuinfo.org, as of 2022-09, all GPUs use 32 bit values for shader handles. This should be ample for future.

    /// @brief A shader binding table is essentially a list of shader references, with some optional custom data (indices etc.) per shader reference
    class ShaderBindingTable
    {
      public:
        /// @brief Add a shader without custom data
        ShaderBindingTable& AddShader(ShaderModule* shader, VkShaderStageFlagBits stage, uint32_t groupId = 0);
        /// @brief Add shader with custom data
        /// @param data pointer to a memory area of mShaderDataSize size. Use SetShaderDataSize(...) before adding shaders!
        ShaderBindingTable& AddShader(ShaderModule* shader, VkShaderStageFlagBits stage, const void* data, uint32_t groupId = 0);

        /// @brief Creates / updates the buffer
        void Build(const VkContext* context, const VkPhysicalDeviceRayTracingPipelinePropertiesKHR& pipelineProperties, const std::vector<ShaderHandle>& handles);

        HSK_PROPERTY_ALLGET(ShaderDataSize)
        HSK_PROPERTY_ALLGET(DeviceAddressRegion)
        HSK_PROPERTY_ALLGET(Buffer)
        HSK_PROPERTY_ALLGET(Shaders)

        void* ShaderDataAt(int32_t index);
        void* ShaderDataAt(int32_t index) const;

        template <typename TData>
        TData& ShaderDataAt(int32_t index)
        {
            return *(reinterpret_cast<TData*>(ShaderDataAt(index)));
        }
        template <typename TData>
        TData& ShaderDataAt(int32_t index) const
        {
            return *(reinterpret_cast<TData*>(ShaderDataAt(index)));
        }

        void WriteToShaderStageCiVector(std::vector<VkPipelineShaderStageCreateInfo>& out) const;
        void GetShaderGroupIds(std::unordered_set<uint32_t>& groupIds) const;
        void GetGroupIdIndex(uint32_t groupId, int32_t& first) const;
        void GetGroupIdIndices(uint32_t groupId, int32_t& first, int32_t& count) const;

        virtual ~ShaderBindingTable();

        struct ShaderReference
        {
            ShaderModule* Module;
            RtShaderType  Type;
            uint          GroupId = {};
        };

      protected:
        ManagedBuffer                   mBuffer;
        VkStridedDeviceAddressRegionKHR mDeviceAddressRegion{};

        VkDeviceSize mShaderDataSize{};

        std::vector<ShaderReference> mShaders;
        std::vector<uint8_t>         mShaderData{};
        RtShaderGroupType            mGroupType = RtShaderGroupType::Undefined;
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

        void Build(const VkContext* context);

        struct ShaderGroup
        {
            uint32_t GroupId;
            int32_t  RaygenIndex;
            int32_t  IntersectsIndexStart;
            int32_t  IntersectsIndexCount;
            int32_t  MissIndex;
            int32_t  CallablesIndexStart;
            int32_t  CallablesIndexCount;
        };

      protected:
        ShaderBindingTable mRaygenSbt;
        ShaderBindingTable mIntersectsSbt;
        ShaderBindingTable mMissSbt;
        ShaderBindingTable mCallablesSbt;

        std::unordered_map<uint32_t, ShaderGroup> mShaderGroups;

        VkPipelineLayout mPipelineLayout;
        VkPipeline       mPipeline;
    };
}  // namespace hsk