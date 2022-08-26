#include "hsk_simplifiedlightmanager.hpp"
#include "../hsk_node.hpp"
#include "../hsk_scene.hpp"

namespace hsk {
    void SimplifiedLightManager::CreateOrUpdate()
    {
        mBuffer.Destroy();
        mSimplifiedlights.clear();
        mComponentArrayBindings.clear();

        std::vector<Node*> nodes;
        GetScene()->FindNodesWithComponent<SimplifiedLightComponent>(nodes);

        for(uint32_t index = 0; index < nodes.size(); index++)
        {
            auto component                     = nodes[index]->GetComponent<SimplifiedLightComponent>();
            mComponentArrayBindings[component] = index;
        }

        mSimplifiedlights.resize(nodes.size());

        VkDeviceSize bufferSize = sizeof(SimplifiedLight) * mSimplifiedlights.size() + 4;

        ManagedBuffer::ManagedBufferCreateInfo ci(VkBufferUsageFlagBits::VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_DST_BIT, bufferSize,
                                                  VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, 0, "Simplifiedlights");
        mBuffer.Create(GetContext(), ci);

        mBuffer.StageSection(0, mSimplifiedlights.data(), 0, sizeof(SimplifiedLight) * mSimplifiedlights.size());
        uint32_t count = mSimplifiedlights.size();
        mBuffer.StageSection(0, &count, sizeof(SimplifiedLight) * mSimplifiedlights.size(), sizeof(count));

        CommandBuffer cmdBuf;
        cmdBuf.Create(GetContext());
        cmdBuf.Begin();
        DualBuffer::DeviceBufferState beforeAndAfter{.AccessFlags        = VkAccessFlagBits::VK_ACCESS_SHADER_READ_BIT,
                                                     .PipelineStageFlags = VkPipelineStageFlagBits::VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
                                                     .QueueFamilyIndex   = VK_QUEUE_FAMILY_IGNORED};
        mBuffer.CmdCopyToDevice(0, cmdBuf, beforeAndAfter, beforeAndAfter);

        cmdBuf.Submit();
    }

    void SimplifiedLightManager::Update(const FrameUpdateInfo& updateInfo)
    {
        for(auto pair : mComponentArrayBindings)
        {
            SimplifiedLightComponent* component = pair.first;
            SimplifiedLight&          simplifiedlight = mSimplifiedlights[pair.second];

            component->UpdateStruct(simplifiedlight);
        }

        mBuffer.StageSection(updateInfo.GetFrameNumber(), mSimplifiedlights.data(), 0, sizeof(SimplifiedLight) * mSimplifiedlights.size());

        DualBuffer::DeviceBufferState beforeAndAfter{.AccessFlags        = VkAccessFlagBits::VK_ACCESS_SHADER_READ_BIT,
                                                     .PipelineStageFlags = VkPipelineStageFlagBits::VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
                                                     .QueueFamilyIndex   = VK_QUEUE_FAMILY_IGNORED};
        mBuffer.CmdCopyToDevice(updateInfo.GetFrameNumber(), updateInfo.GetCommandBuffer(), beforeAndAfter, beforeAndAfter);
    }

}  // namespace hsk