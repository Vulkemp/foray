#pragma once
#include "../base/hsk_vkcontext.hpp"

namespace hsk {

    /// @brief Class to manage a set of descriptors, by creating the descriptor pool, the descriptor set layout and writing the data.
    /// Explanation of vulkan descriptor sets:
    /// They are splitted in two concepts: the descriptor layout and the descriptor pool
    /// The descriptor layout is a list of descriptors, where each descriptor describes the type of resource to be accessed(uniform buffer/image sampler/storage image).
    /// and how many of them as they can also be structured in an array.
    /// The layout is later referenced in the pipeline.
    /// 
    /// The descriptor pool is a chunk of memory that contains a list of pointers to the descriptor sets and
    /// the actual descriptors. If you want 3 ubos and they should for each frame in flight, assuming you have 2 frames in flight you would want to
    /// to have 3 x 2 ubos.
    /// During poolsiz
    class DescriptorSet
    {
      public:
        struct DescriptorInfo
        {
            /// @brief Check if this descriptor set has already been allocated.
            bool Created{false};

            /// @brief The number of descriptors that are allocated by the descriptor pool.
            uint32_t NumberOfDescriptors{1};

            // TODO: DO I need to remember who created the descriptors for this info or can I just deallocate them?
            VkDescriptorSetLayoutBinding DescriptorSetLayoutBinding;
        };

        /// @brief Specify where to place your descriptor.
        /// @param binding
        /// @param set
        /// @param descriptorSetInfo
        void SetDescriptorInfoAt(uint32_t set, uint32_t binding, std::shared_ptr<DescriptorInfo> descriptorInfo);

        /// @brief Creates the descriptor pool, a descriptorset layout and writes the buffer data.
        VkDescriptorSetLayout Create(const VkContext* context, uint32_t maxSets);


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
    };

}  // namespace hsk