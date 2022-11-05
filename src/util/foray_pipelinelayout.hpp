#pragma once

#include "../core/foray_managedresource.hpp"
#include "../foray_basics.hpp"
#include "../foray_vulkan.hpp"
#include <map>
#include <vector>

namespace foray::util {
    /// @brief Class that holds memory ownership of a vulkan pipeline
    class PipelineLayout : public core::VulkanResource<VkObjectType::VK_OBJECT_TYPE_PIPELINE_LAYOUT>
    {
      public:
        PipelineLayout() = default;

        inline virtual bool Exists() const { return !!mPipelineLayout; }

        virtual void Destroy();

        inline ~PipelineLayout()
        {
            Destroy();
        }

        void AddDescriptorSetLayout(VkDescriptorSetLayout layout);
        void AddDescriptorSetLayouts(const std::vector<VkDescriptorSetLayout>& layouts);
        void AddPushConstantRange(VkPushConstantRange range);
        void AddPushConstantRanges(const std::vector<VkPushConstantRange>& ranges);

        template <typename TPushC>
        inline void AddPushConstantRange(VkShaderStageFlags stageFlags, uint32_t offset = ~0U);

        VkPipelineLayout Build(core::Context* context, VkPipelineLayoutCreateFlags flags = 0, void* pNext = nullptr);

        VkPipelineLayout Build(core::Context*                            context,
                                const std::vector<VkDescriptorSetLayout>& descriptorLayouts,
                                const std::vector<VkPushConstantRange>&   pushConstantRanges,
                                VkPipelineLayoutCreateFlags               flags = 0,
                                void*                                     pNext = nullptr);

        FORAY_PROPERTY_GET(PipelineLayout)

        inline operator VkPipelineLayout() const { return mPipelineLayout; }

      protected:
        core::Context*   mContext{};
        VkPipelineLayout mPipelineLayout{};

        std::vector<VkDescriptorSetLayout> mDescriptorSetLayouts;
        std::vector<VkPushConstantRange>   mPushConstantRanges;
        uint32_t mPushConstantOffset = 0U;
    };

    template <typename TPushC>
    inline void PipelineLayout::AddPushConstantRange(VkShaderStageFlags stageFlags, uint32_t offset)
    {
        if (offset == ~0U)
        {
            offset = mPushConstantOffset;
        }
        mPushConstantOffset += sizeof(TPushC);
        VkPushConstantRange range{
            .stageFlags = stageFlags,
            .offset     = offset,
            .size       = sizeof(TPushC),
        };
        AddPushConstantRange(range);
    }

}  // namespace foray::util