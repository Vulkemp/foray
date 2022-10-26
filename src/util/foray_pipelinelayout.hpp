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
        inline void AddPushConstantRange(uint32_t offset = 0U);

        VkPipelineLayout Create(core::Context* context, VkPipelineLayoutCreateFlags flags = 0, void* pNext = nullptr);

        VkPipelineLayout Create(core::Context*                            context,
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
    };

    template <typename TPushC>
    inline void PipelineLayout::AddPushConstantRange(uint32_t offset)
    {
        VkPushConstantRange range{
            .stageFlags = VkShaderStageFlagBits::VK_SHADER_STAGE_COMPUTE_BIT,
            .offset     = offset,
            .size       = sizeof(TPushC),
        };
        AddPushConstantRange(range);
    }

}  // namespace foray::util