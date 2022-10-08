#pragma once
#include "../core/foray_managedbuffer.hpp"
#include "../core/foray_shadermodule.hpp"
#include "foray_rtpipeline_declares.hpp"
#include "foray_rtshadertypes.hpp"
#include <unordered_map>
#include <vector>

namespace foray::rtpipe {

    /// @brief Shader Binding Table base class, providing functions to manage custom generic shader group data and SBT building, aswell as buffer management
    class ShaderBindingTableBase
    {
        /*
          A shader binding table stores entries in the following layout

          | Entry0                      | Entry1                      | ...
          |-----------------------------|-----------------------------|-------
          | GroupHandle | Custom Data   | GroupHandle | Custom Data   | ...

          Where the GroupHandle is a vulkan driver specific memory block (all currently known drivers use 
          32 bit values here), as defined by VkPhysicalDeviceRayTracingPipelinePropertiesKHR::shaderGroupHandleSize
          It is essentially a reference to a ShaderGroup as defined in the CreateInfo passed to 'createRayTracingPipelinesKHR' 

          Custom Data is an optional memory area of user defined size, used to initialize indices. It allows pairing
          shaders with different configuration parameters.

          ShaderBindingTable classes Usage:

            1. If you use custom data, best use SetEntryDataSize(...) first
            2. Define all shader groups. Make sure that all indices from 0 to the highest used are defined!
            3. If you didn't already when defining the shader groups, set custom data now
            4. Use Build(...) to build the SBT. If you use the RtPipeline class, use RtPipeline::Build(...) instead!
        */
      public:
        explicit ShaderBindingTableBase(VkDeviceSize entryDataSize = 0);

        struct VectorRange
        {
            int32_t Start = -1;
            int32_t Count = 0;
        };

        /// @brief Rebuilds the SBT buffer according to the pipeline properties and matching indices
        /// @param context Context
        /// @param pipelineProperties Pipeline Properties for alignment information
        /// @param handles Vector of pointers to handles. Indices in this vector have to match shadergroup indices of the SBT. The caller must guarantee that any memory area pointed to is of pipelineProperties.shaderGroupHandleSize size (bytes)
        virtual void Build(const core::VkContext* context, const VkPhysicalDeviceRayTracingPipelinePropertiesKHR& pipelineProperties, const std::vector<const uint8_t*>& handles);

        FORAY_PROPERTY_CGET(EntryDataSize)
        FORAY_PROPERTY_ALLGET(AddressRegion)
        FORAY_PROPERTY_ALLGET(Buffer)

        /// @brief Access the custom data entry for a group
        std::vector<uint8_t>& GroupDataAt(GroupIndex groupIndex);
        /// @brief Access the custom data entry for a group
        const std::vector<uint8_t>& GroupDataAt(GroupIndex groupIndex) const;

        /// @brief Access the custom data entry for a group. May return nullptr!
        template <typename TData>
        TData* GroupDataAt(GroupIndex groupIndex)
        {
            std::vector<uint8_t>& data = GroupDataAt(groupIndex);
            if(data.size() > 0)
            {
                Assert(sizeof(TData) == data.size(), "Custom data and sizeof(TData) size mismatch!");
                return reinterpret_cast<TData*>(data.data());
            }
            else
            {
                return nullptr;
            }
        }

        /// @brief Access the custom data entry for a group. May return nullptr!
        template <typename TData>
        const TData* GroupDataAt(GroupIndex groupIndex) const
        {
            const std::vector<uint8_t>& data = GroupDataAt(groupIndex);
            if(data.size() > 0)
            {
                Assert(sizeof(TData) == data.size(), "Custom data and sizeof(TData) size mismatch!");
                return reinterpret_cast<const TData*>(data.data());
            }
            else
            {
                return nullptr;
            }
        }

        /// @brief Set the custom data entry for a group
        void SetData(GroupIndex groupIndex, const void* data);

        /// @brief Set the custom data entry for a group
        template <typename TData>
        void SetData(GroupIndex groupIndex, const TData& data)
        {
            Assert(sizeof(TData) == mEntryDataSize);
            SetData(groupIndex, &data);
        }

        /// @brief Set the custom entry data size. Any non-zero custom data is resized according to std::vector<uint8_t>::resize(newSize)!
        virtual ShaderBindingTableBase& SetEntryDataSize(VkDeviceSize newSize);

        /// @brief For any shader group defined, register it with the collection
        virtual void WriteToShaderCollection(RtShaderCollection& collection) const = 0;
        /// @brief Write the shader groups to groupCis vector
        /// @param shaderCollection Collection used to look up the index of shaders
        /// @return the range in the groupCis vector used by this Sbts' ShaderGroups
        virtual VectorRange WriteToShaderGroupCiVector(std::vector<VkRayTracingShaderGroupCreateInfoKHR>& groupCis, const RtShaderCollection& shaderCollection) const = 0;

        virtual void Destroy();

        inline virtual ~ShaderBindingTableBase() { Destroy(); }


      protected:
        core::ManagedBuffer             mBuffer;
        VkStridedDeviceAddressRegionKHR mAddressRegion{};

        VkDeviceSize mEntryDataSize{};

        std::vector<std::vector<uint8_t>> mGroupData{};

        virtual size_t GetGroupArrayCount() const = 0;
        void           ArrayResized(size_t newSize);
    };
}  // namespace foray::rtpipe
