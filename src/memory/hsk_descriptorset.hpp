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
    /// Class description:
    /// You can feed an object of this class with DescriptorInfos.
    /// Each descriptor info describes to which binding it should be attached & to how many sets how many descriptors should be attached.
    /// The descriptor pool will allocate the sets and allocate a descriptor for each.
    class DescriptorSet
    {
      public:
        //
        struct DescriptorInfo
        {
            uint32_t           DescriptorCount{0};
            VkDescriptorType   DescriptorType{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER};
            const VkSampler*   pImmutableSamplers{nullptr};
            VkShaderStageFlags ShaderStageFlags{0};

            // The content to the descriptor sets is delivered in a two dimensional vector, where the size of the first vector specifies the number
            // of sets that are to be configured and the size of the second vector specifies the count of descriptors to be written into each binding.
            // So the first vector size refers to number of sets and the second to the DescriptorCount in the descriptor layout binding.
            // The size of the first vector in buffer/image infos must either be ONE (use same descriptor in all sets)
            // or equal to the amount of sets that shall be created (ONE descriptor per set)
            std::vector<std::vector<VkDescriptorBufferInfo>> BufferInfos{};
            std::vector<std::vector<VkDescriptorImageInfo>>  ImageInfos{};
        };

        /// @brief Specify where to place your descriptor.
        /// @param binding
        /// @param descriptorSetInfo
        void SetDescriptorInfoAt(uint32_t binding, std::shared_ptr<DescriptorInfo> descriptorInfo);

        /// @brief Creates the descriptor pool, a descriptorset layout and writes the buffer data.
        /// @param context
        /// @param numSets
        /// @return
        VkDescriptorSetLayout Create(const VkContext* context, uint32_t numSets);


      protected:
        struct DescriptorLocation
        {
            uint32_t                        Binding;
            std::shared_ptr<DescriptorInfo> Descriptor;
        };
        std::vector<DescriptorLocation> mDescriptorLocations;

        const VkContext*             mContext{};
        VkDescriptorPool             mDescriptorPool{};
        VkDescriptorSetLayout        mDescriptorSetLayout{};
        std::vector<VkDescriptorSet> mDescriptorSets;
    };

}  // namespace hsk