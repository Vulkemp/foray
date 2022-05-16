#pragma once
#include "../base/hsk_vkcontext.hpp"

namespace hsk {

    /// @brief Class to manage a set of descriptors, by creating the descriptor pool, the descriptor set layout and writing the data.
    /// Explanation of vulkan descriptor sets:
    /// 
    /// Distinct between a descriptor, a descriptor set, a descriptor set layout and a descriptor pool.
    /// 
    /// Descriptor:
    /// A descriptor is a structure that holds a pointer to a vulkan resource (buffer or image)
    /// and stores some meta data, about the size and layout. A descriptor can also be an array of resources.
    /// 
    /// Descriptor set:
    /// A descriptor set is a bunch of descriptors together. Descriptors can only be accessed via a set thats bound via vkBindDescriptorSet.
    /// Multiple sets can be bound and accessed in the shader via layout(set=0 / set=1 / set=2, etc.
    /// 
    /// Descriptor set layout:
    /// The layout specifies the order and type of the descriptors bound in a set.
    /// 
    /// Descriptor Pool:
    /// The descriptor pool is a chunk of memory that holds the actual memory for all the descriptors and a list of pointers to the layouts.
    /// Thats why while creating the descriptor pool you have to specify how many layouts can be allocated at max.
    /// 
    class DescriptorSet
    {
      public:

        // 
        struct DescriptorInfo
        {
            /// @brief Check if this descriptor set has already been allocated.
            bool Created{false};

            /// @brief The number of descriptors that are allocated by the descriptor pool.
            uint32_t NumberOfDescriptors{1};

            // TODO: DO I need to remember who created the descriptors for this info or can I just deallocate them?
            VkDescriptorSetLayoutBinding DescriptorSetLayoutBinding;

            // write either buffer or image info
            VkDescriptorBufferInfo BufferInfo{};
            VkDescriptorImageInfo ImageInfo{};
        };

        /// @brief Specify where to place your descriptor.
        /// @param binding
        /// @param set
        /// @param descriptorSetInfo
        void SetDescriptorInfoAt(uint32_t set, uint32_t binding, std::shared_ptr<DescriptorInfo> descriptorInfo);

        /// @brief Creates the descriptor pool, a descriptorset layout and writes the buffer data.
        /// @param context
        /// @param numSets
        /// @return 
        VkDescriptorSetLayout Create(const VkContext* context, uint32_t numSets);


      protected:
        struct DescriptorLocation
        {
            uint32_t                        Set;
            uint32_t                        Binding;
            std::shared_ptr<DescriptorInfo> DescriptorInfos;
        };
        std::vector<DescriptorLocation> mDescriptorLocations;

        const VkContext*      mContext{};
        VkDescriptorPool      mDescriptorPool{};
        VkDescriptorSetLayout mDescriptorSetLayout{};
        std::vector<VkDescriptorSet> mDescriptorSets;
    };

}  // namespace hsk