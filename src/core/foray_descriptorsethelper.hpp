#pragma once
#include "foray_deviceresource.hpp"
#include "foray_vkcontext.hpp"
#include <unordered_map>

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

namespace foray::core {

    /// @brief Class to manage a set of descriptors. It creates the descriptor pool, the descriptor set layout and finally writes the data.
    ///
    /// Class usage:
    /// Step 1: Prepare DescriptorInfo objects.
    /// Step 2: Bind DescriptorInfo to a slot, using SetDescriptorInfoAt
    class DescriptorSetHelper : public DeviceResourceBase
    {
      public:
        /// @brief Each DescriptorInfo corresponds to a descriptor write. It can specify to how many sets it must be written, by adding more values.
        /// ATTENTION: All memory is only referenced (BufferInfos, ImageInfos, pNext Structures) is handled externally
        /// and must be kept valid in order to use the DescriptorInfo.
        class DescriptorInfo
        {
          public:
            void Init(VkDescriptorType type, VkShaderStageFlags shaderStageFlags);
            void Init(VkDescriptorType type, VkShaderStageFlags shaderStageFlags, std::vector<VkDescriptorBufferInfo>* bufferInfosFirstSet);
            void Init(VkDescriptorType type, VkShaderStageFlags shaderStageFlags, std::vector<VkDescriptorImageInfo>* imageInfosFirstSet);

            void AddDescriptorSet(std::vector<VkDescriptorBufferInfo>* bufferInfos);
            void AddDescriptorSet(std::vector<VkDescriptorImageInfo>* imageInfos);

            /// @brief Add a pNext to the descriptor write
            /// @param pNext - A valid pointer to the existing pNextStructure.
            /// @param descriptorCount - The number of objects that are referenced by pNext, aka descriptorCount;
            void AddPNext(void* pNext, uint32_t descriptorCount);


            /// @brief Set pointer to an array of samplers, see immutable samplers description:
            /// https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/VkDescriptorSetLayoutBinding.html#_description
            void SetImmutableSamplers(const VkSampler* immutableSamplers) { mImmutableSamplers = immutableSamplers; }

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
            std::vector<std::vector<VkDescriptorBufferInfo>*> mBufferInfos{};
            std::vector<std::vector<VkDescriptorImageInfo>*>  mImageInfos{};

            /// @brief For acceleration structures etc. an array of pNext pointers can be passed that will be used for the descriptor writes.
            std::vector<void*> mPNextArray;

            friend DescriptorSetHelper;
        };

        /// @brief Specify binding location in your descriptor set.
        void SetDescriptorInfoAt(uint32_t binding, std::shared_ptr<DescriptorInfo> descriptorInfo);

        /// @brief Creates the descriptor pool, a descriptorset layout and writes the buffer data.
        /// @param context - The vulkan context.
        /// @param numSets - If num sets is -1, the number of sets will be automatically determined by all given descriptor infos,
        /// @return The descriptor set layout of the created descriptor sets.
        VkDescriptorSetLayout Create(const VkContext* context, int32_t numSets = -1, std::string name = "Unnamed descriptor set");
        VkDescriptorSetLayout Create(const VkContext* context, std::string name = "Unnamed descriptor set");

        void Update(const VkContext* context);

        void Destroy() override;
        bool Exists() const override { return mDescriptorSets.size(); }

        const std::vector<VkDescriptorSet>& GetDescriptorSets() { return mDescriptorSets; }

        FORAY_PROPERTY_CGET(DescriptorSets)
        FORAY_PROPERTY_CGET(DescriptorSetLayout)

      protected:
        std::unordered_map<uint32_t, std::shared_ptr<DescriptorInfo>> mDescriptorLocations;

        const VkContext*             mContext{};
        VkDescriptorPool             mDescriptorPool{};
        VkDescriptorSetLayout        mDescriptorSetLayout{};
        std::vector<VkDescriptorSet> mDescriptorSets;
        size_t                       mHighestSetCount{1};
    };

}  // namespace foray::core