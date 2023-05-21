#include "descriptorset.hpp"
#include "samplercollection.hpp"

namespace foray::core {
    DescriptorLayout::DescriptorLayout(Context* context, const Builder& builder) : mContext(context), mLayout(nullptr)
    {
        std::vector<VkDescriptorSetLayoutBinding> vkBindings;
        for(uint32_t i = 0; i < builder.GetBindings().size(); i++)
        {
            const DescriptorBindingBase* binding = builder.GetBindings()[i];
            if(!!binding)
            {
                std::string error;
                if(!binding->ValidateForBind(error))
                {
                    FORAY_THROWFMT("Validation of descriptor binding failed: {}", error)
                }
                VkDescriptorSetLayoutBinding& vkBinding = vkBindings.emplace_back(binding->GetBinding());
                vkBinding.binding                       = i;
                DescriptorTypeCountMap::iterator iter   = mPerSetRequirements.find(vkBinding.descriptorType);
                if(iter != mPerSetRequirements.end())
                {
                    mPerSetRequirements[vkBinding.descriptorType] = iter->second + vkBinding.descriptorCount;
                }
                else
                {
                    mPerSetRequirements[vkBinding.descriptorType] = vkBinding.descriptorCount;
                }
                mBindObjToBinding[binding] = i;
            }
        }
        Assert(vkBindings.size() > 0, "Cannot create descriptor layout with no bindings!");
        VkDescriptorSetLayoutCreateInfo ci{
            .sType        = VkStructureType::VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .pNext        = builder.GetPNext(),
            .flags        = builder.GetFlags(),
            .bindingCount = (uint32_t)vkBindings.size(),
            .pBindings    = vkBindings.data(),
        };
        AssertVkResult(mContext->Device->GetDispatchTable().createDescriptorSetLayout(&ci, nullptr, &mLayout));
    }

    DescriptorLayout::~DescriptorLayout()
    {
        mContext->Device->GetDispatchTable().destroyDescriptorSetLayout(mLayout, nullptr);
    }
    DescriptorPool::Builder& DescriptorPool::Builder::AddSets(const DescriptorLayout* layout, uint32_t count)
    {
        for(const auto& kvp : layout->GetPerSetRequirements())
        {
            DescriptorTypeCountMap::iterator iter = mTypeCounts.find(kvp.first);
            if(iter != mTypeCounts.end())
            {
                mTypeCounts[kvp.first] = iter->second + kvp.second * count;
            }
            else
            {
                mTypeCounts[kvp.first] = kvp.second * count;
            }
        }
        mMaxSets += count;
        return *this;
    }

    DescriptorPool::Builder GetUberPoolBuilder()
    {
        DescriptorPool::Builder builder;
        DescriptorTypeCountMap& typeCounts                                          = builder.GetTypeCounts();
        typeCounts[VkDescriptorType::VK_DESCRIPTOR_TYPE_SAMPLER]                    = 0u;
        typeCounts[VkDescriptorType::VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER]     = 256u;
        typeCounts[VkDescriptorType::VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE]              = 0u;
        typeCounts[VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_IMAGE]              = 128u;
        typeCounts[VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER]       = 0u;
        typeCounts[VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER]       = 0u;
        typeCounts[VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER]             = 64u;
        typeCounts[VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_BUFFER]             = 64u;
        typeCounts[VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC]     = 0u;
        typeCounts[VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC]     = 0u;
        typeCounts[VkDescriptorType::VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT]           = 0u;
        typeCounts[VkDescriptorType::VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK]       = 0u;
        typeCounts[VkDescriptorType::VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR] = 16u;
        typeCounts[VkDescriptorType::VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV]  = 0u;
        typeCounts[VkDescriptorType::VK_DESCRIPTOR_TYPE_SAMPLE_WEIGHT_IMAGE_QCOM]   = 0u;
        typeCounts[VkDescriptorType::VK_DESCRIPTOR_TYPE_BLOCK_MATCH_IMAGE_QCOM]     = 0u;
        typeCounts[VkDescriptorType::VK_DESCRIPTOR_TYPE_MUTABLE_EXT]                = 0u;
        typeCounts[VkDescriptorType::VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK_EXT]   = 0u;
        typeCounts[VkDescriptorType::VK_DESCRIPTOR_TYPE_MUTABLE_VALVE]              = 0u;
        builder.SetFlags(VkDescriptorPoolCreateFlagBits::VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT);
        builder.SetMaxSets(32);
        return builder;
    }

    DescriptorPool::DescriptorPool(core::Context* context) : DescriptorPool(context, GetUberPoolBuilder()) {}
    DescriptorPool::DescriptorPool(core::Context* context, const Builder& builder)
        : mContext(context), mAllowFree((builder.GetFlags() & VkDescriptorPoolCreateFlagBits::VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT) != 0)
    {
        std::vector<VkDescriptorPoolSize> poolSizes;
        poolSizes.reserve(builder.GetTypeCounts().size());
        for(const auto& kvp : builder.GetTypeCounts())
        {
            poolSizes.push_back(VkDescriptorPoolSize{.type = kvp.first, .descriptorCount = kvp.second});
        }
        // clang-format off
        VkDescriptorPoolCreateInfo ci{
            .sType = VkStructureType::VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .pNext = builder.GetPNext(),
            .flags = builder.GetFlags(),
            .maxSets = builder.GetMaxSets(),
            .poolSizeCount = (uint32_t)poolSizes.size(),
            .pPoolSizes = poolSizes.data()
        };
        // clang-format on
        AssertVkResult(mContext->Device->GetDispatchTable().createDescriptorPool(&ci, nullptr, &mPool));
    }
    DescriptorPool::~DescriptorPool()
    {
        mContext->Device->GetDispatchTable().destroyDescriptorPool(mPool, nullptr);
    }
    DescriptorLayout::Builder& DescriptorLayout::Builder::AddBinding(DescriptorBindingBase* binding)
    {
        mBindings.push_back(binding);
        return *this;
    }
    DescriptorLayout::Builder& DescriptorLayout::Builder::SetBinding(uint32_t index, DescriptorBindingBase* binding)
    {
        Assert(index >= 0);
        if(index >= mBindings.size())
        {
            mBindings.resize(index + 1);
        }
        mBindings[index] = binding;
        return *this;
    }
    DescriptorSet::DescriptorSet(Context* context, const Builder& builder)
        : mContext(context), mPool(builder.GetPool()), mLayout(builder.GetLayout()), mSet(nullptr), mBindObjToBinding(builder.GetLayout()->GetBindObjToBinding())
    {
        VkDescriptorSetLayout       layouts[] = {mLayout->GetLayout()};
        VkDescriptorSetAllocateInfo allocInfo{
            .sType              = VkStructureType::VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .pNext              = builder.GetPNext(),
            .descriptorPool     = mPool->GetPool(),
            .descriptorSetCount = 1u,
            .pSetLayouts        = layouts,
        };
        AssertVkResult(mContext->Device->GetDispatchTable().allocateDescriptorSets(&allocInfo, &mSet));
    }
    DescriptorSet::~DescriptorSet()
    {
        if(mPool->GetAllowFree())
        {
            mContext->Device->GetDispatchTable().freeDescriptorSets(mPool->GetPool(), 1u, &mSet);
        }
    }
    void DescriptorSet::Update(const UpdateInfo& update)
    {
        Assert((update.GetVkWrites().size() + update.GetBindingWrites().size()) > 0, "No writes configured!");

        std::vector<VkWriteDescriptorSet> writes;
        writes.reserve(update.GetVkWrites().size() + update.GetBindingWrites().size());
        for(const VkWriteDescriptorSet& write : update.GetVkWrites())
        {
            VkWriteDescriptorSet copy(write);
            copy.dstSet = mSet;
            writes.push_back(copy);
        }
        for(const DescriptorBindingBase* binding : update.GetBindingWrites())
        {
            if (!binding->ShouldWrite())
            { // In case of an immutable sampler, writing is illegal
                continue;
            }
            VkWriteDescriptorSet copy(binding->GetState());
            copy.dstSet     = mSet;
            copy.dstBinding = mBindObjToBinding[binding];
            writes.push_back(copy);
        }

        mContext->Device->GetDispatchTable().updateDescriptorSets((uint32_t)writes.size(), writes.data(), 0, nullptr);
    }
    DescriptorSet::UpdateInfo& DescriptorSet::UpdateInfo::AddWrite(const VkWriteDescriptorSet& write)
    {
        mVkWrites.push_back(write);
        return *this;
    }
    DescriptorSet::UpdateInfo& DescriptorSet::UpdateInfo::AddWrite(const core::DescriptorBindingBase* binding)
    {
        std::string error;
        if (!binding->ValidateForWrite(error))
        {
            FORAY_THROWFMT("DescriptorWrite validation failed {}", error)
        }
        mBindingWrites.push_back(binding);
        return *this;
    }
}  // namespace foray::core