#pragma once
#include "../core/managedbuffer.hpp"
#include "../core/shadermodule.hpp"
#include "../mem.hpp"
#include "rtpipeline_declares.hpp"
#include "rtshadertypes.hpp"
#include <span>
#include <unordered_map>
#include <vector>

namespace foray::rtpipe {

    /// @brief Shader Binding Table base class, providing functions to manage custom generic shader group data and SBT building, aswell as buffer management
    /// @details
    /// A shader binding table stores entries in the following layout
    ///
    /// ```
    /// | Entry0                      | Entry1                      | ...
    /// |-----------------------------|-----------------------------|-------
    /// | GroupHandle | Custom Data   | GroupHandle | Custom Data   | ...
    /// ```
    ///
    /// Where the GroupHandle is a vulkan driver specific memory block (all currently known drivers use
    /// 32 bit values here), as defined by VkPhysicalDeviceRayTracingPipelinePropertiesKHR::shaderGroupHandleSize
    /// It is essentially a reference to a ShaderGroup as defined in the CreateInfo passed to 'createRayTracingPipelinesKHR'
    ///
    /// Custom Data is an optional memory area of user defined size. It allows pairing
    /// shaders with different configuration parameters.
    ///
    /// ShaderBindingTable classes Usage:
    ///
    ///   1. If you use custom data, best use SetEntryDataSize(...) first
    ///   2. Define all shader groups. Make sure that all indices from 0 to the highest used are defined!
    ///   3. If you didn't already when defining the shader groups, set custom data now
    ///   4. Use Build(...) to build the SBT. If you use the RtPipeline class, use RtPipeline::Build(...) instead!
    class ShaderBindingTableBase
    {
      public:
        struct VectorRange
        {
            int32_t Start = -1;
            int32_t Count = 0;
        };

        class Builder
        {
          public:
            Builder& SetEntryData(int32_t groupIdx, const void* data, std::size_t size);
            template <typename T>
            Builder& SetEntryData(int32_t groupIdx, const T& data)
            {
                SetEntryData(groupIdx, &data, sizeof(T));
            }

            std::span<const uint8_t> GetEntryData(int32_t groupIdx) const;


            FORAY_PROPERTY_V(EntryDataSize)
            FORAY_PROPERTY_R(GroupData)
            FORAY_PROPERTY_V(SgHandles)
            FORAY_PROPERTY_V(PipelineProperties)

          private:
            VkDeviceSize                                           mEntryDataSize = {};
            std::vector<std::vector<uint8_t>>                      mGroupData = {};
            const std::vector<const uint8_t*>*                     mSgHandles = nullptr;
            const VkPhysicalDeviceRayTracingPipelinePropertiesKHR* mPipelineProperties = nullptr;
        };

        virtual ~ShaderBindingTableBase() = default;

        /// @brief Rebuilds the SBT buffer according to the pipeline properties and matching indices
        /// @param context Context
        ShaderBindingTableBase(core::Context* context, const Builder& builder);

        FORAY_GETTER_V(AddressRegion)
      protected:
        Local<core::ManagedBuffer>      mBuffer;
        VkStridedDeviceAddressRegionKHR mAddressRegion{};
    };
}  // namespace foray::rtpipe
