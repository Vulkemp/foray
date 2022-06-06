#include "hsk_descriptorsethelper.hpp"
#include "../hsk_vkHelpers.hpp"

namespace hsk {
    void DescriptorSetHelper::SetDescriptorInfoAt(uint32_t binding, std::shared_ptr<DescriptorInfo> descriptorSetInfo)
    {
        mDescriptorLocations.push_back({binding, descriptorSetInfo});
        mHighestSetCount = std::max(mHighestSetCount, descriptorSetInfo->mBufferInfos.size());
        mHighestSetCount = std::max(mHighestSetCount, descriptorSetInfo->mImageInfos.size());
    }

    VkDescriptorSetLayout DescriptorSetHelper::Create(const VkContext* context, int32_t numSets)
    {
        // auto detect set count if necessary
        numSets = numSets != -1 ? numSets : mHighestSetCount;

        mContext = context;
        // --------------------------------------------------------------------------------------------
        // create the descriptor set layout based on the given bindings
        std::vector<VkDescriptorSetLayoutBinding> layoutBindings;
        layoutBindings.reserve(mDescriptorLocations.size());

        for(size_t i = 0; i < mDescriptorLocations.size(); i++)
        {
            auto& descriptorLocation = mDescriptorLocations[i];

            VkDescriptorSetLayoutBinding layoutBinding{};
            layoutBinding.binding            = descriptorLocation.Binding;
            layoutBinding.descriptorCount    = descriptorLocation.Descriptor->mDescriptorCount;
            layoutBinding.descriptorType     = descriptorLocation.Descriptor->mDescriptorType;
            layoutBinding.pImmutableSamplers = descriptorLocation.Descriptor->mImmutableSamplers;
            layoutBinding.stageFlags         = descriptorLocation.Descriptor->mShaderStageFlags;

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
            poolSize.type            = descriptorLocation.Descriptor->mDescriptorType;
            poolSize.descriptorCount = descriptorLocation.Descriptor->mDescriptorCount * numSets;

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
                descriptorWrite.dstArrayElement  = 0;  // This offsets into descriptorLocation.DescriptorInfo->BufferInfos/ImageInfos, which always starts at 0.
                descriptorWrite.descriptorType   = descriptorLocation.Descriptor->mDescriptorType;
                descriptorWrite.descriptorCount  = 0;  // number of descriptors (used for descriptor arrays), set below individually
                descriptorWrite.pBufferInfo      = nullptr;
                descriptorWrite.pImageInfo       = nullptr;
                descriptorWrite.pTexelBufferView = nullptr;  // TODO: whats a TexelBufferView?

                uint32_t numBufferInfos = descriptorLocation.Descriptor->mBufferInfos.size();
                if(descriptorLocation.Descriptor->mBufferInfos.size() > 0)
                {

                    // if only one buffer info specified, use the first(0) for all descriptor sets, otherwise use set index i
                    uint32_t index                  = numBufferInfos > 1 ? setIndex : 0;
                    descriptorWrite.descriptorCount = descriptorLocation.Descriptor->mBufferInfos[index].size();
                    descriptorWrite.pBufferInfo     = descriptorLocation.Descriptor->mBufferInfos[index].data();
                    descriptorWrites.push_back(descriptorWrite);
                    continue;
                }

                uint32_t numImageInfos = descriptorLocation.Descriptor->mImageInfos.size();
                if(numImageInfos > 0)
                {
                    // if only one image info specified, use the first(0) for all descriptor sets, otherwise use set index i
                    uint32_t index                  = numImageInfos > 1 ? setIndex : 0;
                    descriptorWrite.descriptorCount = descriptorLocation.Descriptor->mImageInfos[index].size();
                    descriptorWrite.pImageInfo      = descriptorLocation.Descriptor->mImageInfos[index].data();
                }
                descriptorWrites.push_back(descriptorWrite);
            }
        }
        vkUpdateDescriptorSets(context->Device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
        return mDescriptorSetLayout;
    }

    void DescriptorSetHelper::Cleanup()
    {
        if(mDescriptorSetLayout)
        {
            vkDestroyDescriptorSetLayout(mContext->Device, mDescriptorSetLayout, nullptr);
            mDescriptorSetLayout = nullptr;
        }
        if(mDescriptorPool)
        {
            vkDestroyDescriptorPool(mContext->Device, mDescriptorPool, nullptr);
            mDescriptorPool = nullptr;
        }
        mDescriptorSets.resize(0);
    }


    void DescriptorSetHelper::DescriptorInfo::Init(VkDescriptorType type, VkShaderStageFlags shaderStageFlags) {
        mDescriptorType   = type;
        mShaderStageFlags = shaderStageFlags;
    }

    void DescriptorSetHelper::DescriptorInfo::Init(VkDescriptorType type, VkShaderStageFlags shaderStageFlags, std::vector<VkDescriptorBufferInfo>& bufferInfosFirstSet)
    {
        mDescriptorType   = type;
        mShaderStageFlags = shaderStageFlags;
        mBufferInfos.push_back(bufferInfosFirstSet);
        mDescriptorCount = bufferInfosFirstSet.size();
    }

    void DescriptorSetHelper::DescriptorInfo::Init(VkDescriptorType type, VkShaderStageFlags shaderStageFlags, std::vector<VkDescriptorImageInfo>& imageInfosFirstSet)
    {
        mDescriptorType   = type;
        mShaderStageFlags = shaderStageFlags;
        mImageInfos.push_back(imageInfosFirstSet);
        mDescriptorCount = imageInfosFirstSet.size();
    }

    void DescriptorSetHelper::DescriptorInfo::AddDescriptorSet(std::vector<VkDescriptorBufferInfo>& bufferInfos)
    {
        size_t countDescriptorInfos = bufferInfos.size();
        if(mDescriptorCount > 0)
        {
            Assert(mDescriptorCount != countDescriptorInfos,
                   "Cannot add buffer infos with a different amount of descriptor handles! All buffer info vectors need to be of the same size!");
        }
        else
        {
            // first set added
            mDescriptorCount = static_cast<uint32_t>(countDescriptorInfos);
        }

        mBufferInfos.push_back(bufferInfos);
    }

    void DescriptorSetHelper::DescriptorInfo::AddDescriptorSet(std::vector<VkDescriptorImageInfo>& imageInfos)
    {
        size_t countDescriptorInfos = imageInfos.size();
        if(mDescriptorCount > 0)
        {
            Assert(mDescriptorCount != countDescriptorInfos,
                   "Cannot add image infos with a different amount of descriptor handles! All image info vectors need to be of the same size!");
        }
        else
        {
            // first set added
            mDescriptorCount = static_cast<uint32_t>(countDescriptorInfos);
        }
        mImageInfos.push_back(imageInfos);
    }
}  // namespace hsk
