#pragma once

#include "../core/foray_managedresource.hpp"
#include "../foray_basics.hpp"
#include "../foray_vulkan.hpp"
#include <map>
#include <vector>

namespace foray::util {
    /// @brief Class that holds memory ownership of a vulkan pipeline layout
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

        static const uint32_t PUSHC_OFFSET_AUTO = ~0U;

        /// @brief Add a descriptorset layout to the pipeline layout prior to building
        void AddDescriptorSetLayout(VkDescriptorSetLayout layout);
        /// @brief Add descriptorset layouts to the pipeline layout prior to building
        void AddDescriptorSetLayouts(const std::vector<VkDescriptorSetLayout>& layouts);
        /// @brief Add a push constant range to the pipeline layout prior to building
        /// @remark If range.offset is set to PUSHC_OFFSET_AUTO, it is automatically set based on previously added pushconstants
        void AddPushConstantRange(VkPushConstantRange range);
        /// @brief Add push constant ranges to the pipeline layout prior to building
        /// @remark If range.offset is set to PUSHC_OFFSET_AUTO, it is automatically set based on previously added pushconstants
        void AddPushConstantRanges(const std::vector<VkPushConstantRange>& ranges);
        
        /// @brief Add push constant ranges to the pipeline layout prior to building
        /// @tparam TPushC Push Constant type (determines size)
        /// @param stageFlags Shader Stage Flags
        /// @param offset Offset. If set to PUSHC_OFFSET_AUTO, it is automatically set based on previously added pushconstants
        template <typename TPushC>
        inline void AddPushConstantRange(VkShaderStageFlags stageFlags, uint32_t offset = PUSHC_OFFSET_AUTO);

        /// @brief Builds the pipelinelayout based on previously added descriptorset layouts and push constant ranges
        /// @param context Requires DispatchTable
        /// @param flags VkPipelineLayoutCreateInfo::flags
        /// @param pNext VkPipelineLayoutCreateInfo::pNext
        VkPipelineLayout Build(core::Context* context, VkPipelineLayoutCreateFlags flags = 0, void* pNext = nullptr);

        /// @brief Builds the pipelinelayout based on previously added descriptorset layouts and push constant ranges (aswell as parameters)
        /// @param context Requires DispatchTable
        /// @param descriptorLayouts Additional DescriptorSetLayouts
        /// @param pushConstantRanges Additional PushConstantRanges
        /// @param flags VkPipelineLayoutCreateInfo::flags
        /// @param pNext VkPipelineLayoutCreateInfo::pNext
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