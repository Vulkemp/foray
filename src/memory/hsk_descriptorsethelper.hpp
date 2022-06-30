#pragma once
#include "../base/hsk_vkcontext.hpp"

/// Explanation of vulkan descriptor sets:
/// Distinction between a descriptor, a descriptor set, a descriptor set layout and a descriptor pool.
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

namespace hsk {

    /// @brief Class to manage a set of descriptors. It creates the descriptor pool, the descriptor set layout and finally writes the data.
    /// 
    /// Class usage:
    /// Step 1: Prepare DescriptorInfo objects.
    /// Step 2: Bind DescriptorInfo to a slot, using SetDescriptorInfoAt
    class DescriptorSetHelper
    {
      public:
 
        /// @brief To use this class, use one of the init methods to set descriptor type, shaderstage flags and the descriptor handles.
        class DescriptorInfo
        {
          public:
            void Init(VkDescriptorType type, VkShaderStageFlags shaderStageFlags);
            void Init(VkDescriptorType type, VkShaderStageFlags shaderStageFlags, std::vector<VkDescriptorBufferInfo>& bufferInfosFirstSet);
            void Init(VkDescriptorType type, VkShaderStageFlags shaderStageFlags, std::vector<VkDescriptorImageInfo>& imageInfosFirstSet);

            void AddDescriptorSet(const std::vector<VkDescriptorBufferInfo>& bufferInfos);
            void AddDescriptorSet(const std::vector<VkDescriptorImageInfo>& imageInfos);

            /// @brief Set pointer to an array of samplers, see immutable samplers description:
            /// https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/VkDescriptorSetLayoutBinding.html#_description
            void SetImmutableSamplers(const VkSampler* immutableSamplers) { mImmutableSamplers = immutableSamplers; }

            /// @brief Use this to write all descriptor set data manually.
            std::vector<std::vector<VkDescriptorBufferInfo>>& GetBufferInfos() { return mBufferInfos; }
            std::vector<std::vector<VkDescriptorImageInfo>>&  GetImageInfos() { return mImageInfos; }
          protected:
            /// @brief The amount of descriptors this binding has attached in each set.
            uint32_t           mDescriptorCount{0};
            VkDescriptorType   mDescriptorType{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER};
            const VkSampler*   mImmutableSamplers{nullptr};
            VkShaderStageFlags mShaderStageFlags{0};

            // The content to the descriptor sets is delivered in a two dimensional vector, where the size of the first vector specifies the number
            // of sets that are to be configured and the size of the second vector specifies the count of descriptors to be written into each binding.
            // So the first vector size refers to number of sets and the second to the DescriptorCount in the descriptor layout binding.
            // The size of the first vector in buffer/image infos must either be ONE (use same descriptor in all sets)
            // or equal to the amount of sets that shall be created (ONE descriptor per set)
            std::vector<std::vector<VkDescriptorBufferInfo>> mBufferInfos{};
            std::vector<std::vector<VkDescriptorImageInfo>>  mImageInfos{};

            friend DescriptorSetHelper;
        };

        /// @brief Specify binding location in your descriptor set.
        void SetDescriptorInfoAt(uint32_t binding, std::shared_ptr<DescriptorInfo> descriptorInfo);

        /// @brief Creates the descriptor pool, a descriptorset layout and writes the buffer data.
        /// @param context - The vulkan context.
        /// @param numSets - If num sets is -1, the number of sets will be automatically determined by all given descriptor infos,
        /// @return The descriptor set layout of the created descriptor sets.
        VkDescriptorSetLayout Create(const VkContext* context, int32_t numSets = -1);

        void Cleanup();

        const std::vector<VkDescriptorSet>& GetDescriptorSets() { return mDescriptorSets; }

        HSK_PROPERTY_CGET(DescriptorSets)
        HSK_PROPERTY_CGET(DescriptorSetLayout)

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
        size_t                       mHighestSetCount{1};
    };

}  // namespace hsk