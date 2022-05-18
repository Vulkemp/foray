#include "hsk_descriptorset.hpp"
#include "../hsk_vkHelpers.hpp"

namespace hsk {
    void DescriptorSet::SetDescriptorInfoAt(uint32_t binding, std::shared_ptr<DescriptorInfo> descriptorSetInfo) { mDescriptorLocations.push_back({binding, descriptorSetInfo}); }

    VkDescriptorSetLayout DescriptorSet::Create(const VkContext* context, uint32_t numSets)
    {
        // --------------------------------------------------------------------------------------------
        // create the descriptor set layout based on the given bindings
        std::vector<VkDescriptorSetLayoutBinding> layoutBindings;
        layoutBindings.reserve(mDescriptorLocations.size());

        for(size_t i = 0; i < mDescriptorLocations.size(); i++)
        {
            auto& descriptorLocation = mDescriptorLocations[i];

            VkDescriptorSetLayoutBinding layoutBinding{};
            layoutBinding.binding = descriptorLocation.Binding;
            layoutBinding.descriptorCount = descriptorLocation.Descriptor->DescriptorCount;
            layoutBinding.descriptorType = descriptorLocation.Descriptor->DescriptorType;
            layoutBinding.pImmutableSamplers = descriptorLocation.Descriptor->pImmutableSamplers;
            layoutBinding.stageFlags = descriptorLocation.Descriptor->ShaderStageFlags;

            layoutBindings.push_back(layoutBinding);
        }

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = mDescriptorLocations.size();
        layoutInfo.pBindings    = layoutBindings.data();

        AssertVkResult(vkCreateDescriptorSetLayout(context->Device, &layoutInfo, nullptr, &mDescriptorSetLayout));

        // --------------------------------------------------------------------------------------------
        // define which descriptors need to be allocated from a descriptor pool, based on the created
        // flag in the shared descriptor info.
        std::vector<VkDescriptorPoolSize> poolSizes;
        poolSizes.reserve(mDescriptorLocations.size());

        for(auto& descriptorLocation : mDescriptorLocations)
        {
            VkDescriptorPoolSize poolSize{};
            poolSize.type            = descriptorLocation.Descriptor->DescriptorType;
            poolSize.descriptorCount = descriptorLocation.Descriptor->DescriptorCount * numSets;

            // prepare pool size
            poolSizes.push_back(poolSize);
        }

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = poolSizes.size();
        poolInfo.pPoolSizes    = poolSizes.data();
        poolInfo.maxSets       = static_cast<uint32_t>(numSets);

        AssertVkResult(vkCreateDescriptorPool(context->Device, &poolInfo, nullptr, &mDescriptorPool));

        // --------------------------------------------------------------------------------------------
        // allocate descriptor sets by their layout

        std::vector<VkDescriptorSetLayout> layouts(numSets, mDescriptorSetLayout);
        VkDescriptorSetAllocateInfo        descriptorSetAllocInfo{};
        descriptorSetAllocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        descriptorSetAllocInfo.descriptorPool     = mDescriptorPool;
        descriptorSetAllocInfo.descriptorSetCount = numSets;
        descriptorSetAllocInfo.pSetLayouts        = layouts.data();

        mDescriptorSets.resize(numSets);
        AssertVkResult(vkAllocateDescriptorSets(context->Device, &descriptorSetAllocInfo, mDescriptorSets.data()));


        // --------------------------------------------------------------------------------------------
        // connect the descriptor sets with the descriptors
        std::vector<VkWriteDescriptorSet> descriptorWrites;

        for(uint32_t setIndex = 0; setIndex < numSets; setIndex++)
        {
            for(auto& descriptorLocation : mDescriptorLocations)
            {
                VkWriteDescriptorSet descriptorWrite{};
                descriptorWrite.sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptorWrite.dstSet           = mDescriptorSets[setIndex];
                descriptorWrite.dstBinding       = descriptorLocation.Binding;
                descriptorWrite.dstArrayElement  = 0; // This offsets into descriptorLocation.DescriptorInfo->BufferInfos/ImageInfos, which always starts at 0.
                descriptorWrite.descriptorType   = descriptorLocation.Descriptor->DescriptorType;
                descriptorWrite.descriptorCount  = 0;
                descriptorWrite.pBufferInfo      = nullptr;
                descriptorWrite.pImageInfo       = nullptr;
                descriptorWrite.pTexelBufferView = nullptr; // TODO: whats a TexelBufferView?

                uint32_t numBufferInfos = descriptorLocation.Descriptor->BufferInfos.size();
                if (descriptorLocation.Descriptor->BufferInfos.size() > 0) {
                    
                    // if only one buffer info specified, use the first(0) for all descriptor sets, otherwise use set index i
                    uint32_t index = numBufferInfos > 1 ? setIndex : 0;
                    descriptorWrite.descriptorCount = descriptorLocation.Descriptor->BufferInfos[index].size();
                    descriptorWrite.pBufferInfo = descriptorLocation.Descriptor->BufferInfos[index].data();
                    continue;
                }

                uint32_t numImageInfos = descriptorLocation.Descriptor->ImageInfos.size();
                if(numImageInfos > 0)
                {
                    // if only one image info specified, use the first(0) for all descriptor sets, otherwise use set index i
                    uint32_t index                  = numImageInfos > 1 ? setIndex : 0;
                    descriptorWrite.descriptorCount = descriptorLocation.Descriptor->BufferInfos[index].size();
                    descriptorWrite.pImageInfo     = descriptorLocation.Descriptor->ImageInfos[index].data();
                }
            }
        }
        vkUpdateDescriptorSets(context->Device, descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
    }
}  // namespace hsk
