#include "hsk_descriptorsethelper.hpp"
#include "../hsk_vkHelpers.hpp"

namespace hsk {
    void DescriptorSetHelper::SetDescriptorInfoAt(uint32_t binding, std::shared_ptr<DescriptorInfo> descriptorSetInfo)
    {
        mDescriptorLocations[binding] = descriptorSetInfo;
        mHighestSetCount = std::max(mHighestSetCount, descriptorSetInfo->mBufferInfos.size());
        mHighestSetCount = std::max(mHighestSetCount, descriptorSetInfo->mImageInfos.size());
    }

    VkDescriptorSetLayout DescriptorSetHelper::Create(const VkContext* context, int32_t numSets, std::string name)
    {
        // set name for debugging
        mName = name;

        // auto detect set count if necessary
        numSets = numSets != -1 ? numSets : mHighestSetCount;

        mContext = context;
        // --------------------------------------------------------------------------------------------
        // create the descriptor set layout based on the given bindings
        std::vector<VkDescriptorSetLayoutBinding> layoutBindings;
        layoutBindings.reserve(mDescriptorLocations.size());

        for(const auto& descriptorLocation : mDescriptorLocations)
        {
            VkDescriptorSetLayoutBinding layoutBinding{};
            layoutBinding.binding            = descriptorLocation.first;
            layoutBinding.descriptorCount    = descriptorLocation.second->mDescriptorCount;
            layoutBinding.descriptorType     = descriptorLocation.second->mDescriptorType;
            layoutBinding.pImmutableSamplers = descriptorLocation.second->mImmutableSamplers;
            layoutBinding.stageFlags         = descriptorLocation.second->mShaderStageFlags;

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

        for(const auto& descriptorLocation : mDescriptorLocations)
        {
            VkDescriptorPoolSize poolSize{};
            poolSize.type            = descriptorLocation.second->mDescriptorType;
            poolSize.descriptorCount = descriptorLocation.second->mDescriptorCount * numSets;

            if(poolSize.descriptorCount)
            {
                // prepare pool size
                poolSizes.push_back(poolSize);
            }
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
        Update(context);
        return mDescriptorSetLayout;
    }

    void DescriptorSetHelper::Update(const VkContext* context)
    {
        std::vector<VkWriteDescriptorSet> descriptorWrites;

        for(uint32_t setIndex = 0; setIndex < mDescriptorSets.size(); setIndex++)
        {
            for(const auto& descriptorLocation : mDescriptorLocations)
            {
                VkWriteDescriptorSet descriptorWrite{};
                descriptorWrite.sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptorWrite.dstSet           = mDescriptorSets[setIndex];
                descriptorWrite.dstBinding       = descriptorLocation.first;
                descriptorWrite.dstArrayElement  = 0;  // This offsets into descriptorLocation.DescriptorInfo->BufferInfos/ImageInfos, which always starts at 0 in this class.
                descriptorWrite.descriptorType   = descriptorLocation.second->mDescriptorType;
                descriptorWrite.descriptorCount  = 0;  // number of descriptors (used for descriptor arrays), set below individually
                descriptorWrite.pBufferInfo      = nullptr;
                descriptorWrite.pImageInfo       = nullptr;
                descriptorWrite.pTexelBufferView = nullptr;  // TODO: whats a TexelBufferView?

                uint32_t numBufferInfos = descriptorLocation.second->mBufferInfos.size();
                if(descriptorLocation.second->mBufferInfos.size() > 0)
                {

                    // if only one buffer info specified, use the first(0) for all descriptor sets, otherwise use set index i
                    uint32_t index                  = numBufferInfos > 1 ? setIndex : 0;
                    descriptorWrite.descriptorCount = descriptorLocation.second->mBufferInfos[index]->size();
                    descriptorWrite.pBufferInfo     = descriptorLocation.second->mBufferInfos[index]->data();
                    descriptorWrites.push_back(descriptorWrite);
                    continue;
                }

                uint32_t numImageInfos = descriptorLocation.second->mImageInfos.size();
                if(numImageInfos > 0)
                {
                    // if only one image info specified, use the first(0) for all descriptor sets, otherwise use set index i
                    uint32_t index                  = numImageInfos > 1 ? setIndex : 0;
                    descriptorWrite.descriptorCount = descriptorLocation.second->mImageInfos[index]->size();
                    descriptorWrite.pImageInfo      = descriptorLocation.second->mImageInfos[index]->data();
                }

                uint32_t numPNext = descriptorLocation.second->mPNextArray.size();
                if(numPNext > 0)
                {
                    // set pNext
                    uint32_t index                  = numPNext > 1 ? setIndex : 0;
                    descriptorWrite.descriptorCount = descriptorLocation.second->mDescriptorCount;
                    descriptorWrite.pNext           = descriptorLocation.second->mPNextArray[index];
                }

                if(descriptorWrite.descriptorCount)
                {
                    descriptorWrites.push_back(descriptorWrite);
                }
            }
        }
        vkUpdateDescriptorSets(context->Device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }

    VkDescriptorSetLayout DescriptorSetHelper::Create(const VkContext* context, std::string name) { return Create(context, -1, name); }

    void DescriptorSetHelper::Destroy()
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


    void DescriptorSetHelper::DescriptorInfo::Init(VkDescriptorType type, VkShaderStageFlags shaderStageFlags)
    {
        mDescriptorType   = type;
        mShaderStageFlags = shaderStageFlags;
    }

    void DescriptorSetHelper::DescriptorInfo::Init(VkDescriptorType type, VkShaderStageFlags shaderStageFlags, std::vector<VkDescriptorBufferInfo>* bufferInfosFirstSet)
    {
        mDescriptorType   = type;
        mShaderStageFlags = shaderStageFlags;
        mBufferInfos.push_back(bufferInfosFirstSet);
        mDescriptorCount = bufferInfosFirstSet->size();
    }

    void DescriptorSetHelper::DescriptorInfo::Init(VkDescriptorType type, VkShaderStageFlags shaderStageFlags, std::vector<VkDescriptorImageInfo>* imageInfosFirstSet)
    {
        mDescriptorType   = type;
        mShaderStageFlags = shaderStageFlags;
        mImageInfos.push_back(imageInfosFirstSet);
        mDescriptorCount = imageInfosFirstSet->size();
    }

    void DescriptorSetHelper::DescriptorInfo::AddDescriptorSet(std::vector<VkDescriptorBufferInfo>* bufferInfos)
    {
        size_t countDescriptorInfos = bufferInfos->size();
        if(mDescriptorCount > 0)
        {
            Assert(mDescriptorCount == countDescriptorInfos,
                   "Cannot add buffer infos with a different amount of descriptor handles! All buffer info vectors need to be of the same size for each added set!");
        }
        else
        {
            // first set added
            mDescriptorCount = static_cast<uint32_t>(countDescriptorInfos);
        }

        mBufferInfos.push_back(bufferInfos);
    }

    void DescriptorSetHelper::DescriptorInfo::AddDescriptorSet(std::vector<VkDescriptorImageInfo>* imageInfos)
    {
        size_t countDescriptorInfos = imageInfos->size();
        if(mDescriptorCount > 0)
        {
            Assert(mDescriptorCount == countDescriptorInfos,
                   "Cannot add image infos with a different amount of descriptor handles! All image info vectors need to be of the same size for each added set!");
        }
        else
        {
            // first set added
            mDescriptorCount = static_cast<uint32_t>(countDescriptorInfos);
        }
        mImageInfos.push_back(imageInfos);
    }
    void DescriptorSetHelper::DescriptorInfo::AddPNext(void* pNext, uint32_t descriptorCount)
    {
        mDescriptorCount = descriptorCount;
        mPNextArray.push_back(pNext);
    }
}  // namespace hsk
