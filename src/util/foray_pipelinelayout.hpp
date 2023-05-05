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
        virtual ~PipelineLayout();

        static const uint32_t PUSHC_OFFSET_AUTO = ~0U;

        class Builder
        {
          public:
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

            FORAY_GETTER_CR(DescriptorSetLayouts)
            FORAY_GETTER_CR(PushConstantRanges)
            FORAY_PROPERTY_V(Flags)
            FORAY_PROPERTY_V(PNext)

          private:
            std::vector<VkDescriptorSetLayout> mDescriptorSetLayouts;
            std::vector<VkPushConstantRange>   mPushConstantRanges;
            uint32_t                           mPushConstantOffset = 0U;
            VkPipelineLayoutCreateFlags mFlags = 0;
            void *mPNext = nullptr;
        };

        /// @brief Builds the pipelinelayout based on previously added descriptorset layouts and push constant ranges
        /// @param context Requires DispatchTable
        /// @param flags VkPipelineLayoutCreateInfo::flags
        /// @param pNext VkPipelineLayoutCreateInfo::pNext
        PipelineLayout(core::Context* context, const Builder& builder);

        FORAY_GETTER_V(PipelineLayout)

        inline operator VkPipelineLayout() const { return mPipelineLayout; }

      protected:
        core::Context*   mContext{};
        VkPipelineLayout mPipelineLayout{};
    };

    template <typename TPushC>
    inline void PipelineLayout::Builder::AddPushConstantRange(VkShaderStageFlags stageFlags, uint32_t offset)
    {
        if(offset == ~0U)
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