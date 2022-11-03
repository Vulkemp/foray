#include "foray_lightmanager.hpp"
#include "../foray_node.hpp"
#include "../foray_scene.hpp"

namespace foray::scene::gcomp {
    void LightManager::CreateOrUpdate()
    {
        mBuffer.Destroy();
        mSimplifiedlights.clear();
        mComponentArrayBindings.clear();

        std::vector<Node*> nodes;
        GetScene()->FindNodesWithComponent<ncomp::PunctualLight>(nodes);

        for(uint32_t index = 0; index < nodes.size(); index++)
        {
            auto component                     = nodes[index]->GetComponent<ncomp::PunctualLight>();
            mComponentArrayBindings[component] = index;
        }

        mSimplifiedlights.resize(nodes.size());

        VkDeviceSize bufferSize = sizeof(SimpleLight) * mSimplifiedlights.size() + 4;

        core::ManagedBuffer::CreateInfo ci(VkBufferUsageFlagBits::VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                                        bufferSize, VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, 0, "Simplifiedlights");
        mBuffer.Create(GetContext(), ci);

        mBuffer.StageSection(0, mSimplifiedlights.data(), 0, sizeof(SimpleLight) * mSimplifiedlights.size());
        uint32_t count = mSimplifiedlights.size();
        mBuffer.StageSection(0, &count, sizeof(SimpleLight) * mSimplifiedlights.size(), sizeof(count));

        core::HostCommandBuffer cmdBuf;
        cmdBuf.Create(GetContext());
        cmdBuf.Begin();
        util::DualBuffer::DeviceBufferState beforeAndAfter{.AccessFlags        = VkAccessFlagBits::VK_ACCESS_SHADER_READ_BIT,
                                                           .PipelineStageFlags = VkPipelineStageFlagBits::VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
                                                           .QueueFamilyIndex   = VK_QUEUE_FAMILY_IGNORED};
        mBuffer.CmdCopyToDevice(0, cmdBuf, beforeAndAfter, beforeAndAfter);

        cmdBuf.SubmitAndWait();
    }

    void LightManager::Update(SceneUpdateInfo& updateInfo)
    {
        for(auto pair : mComponentArrayBindings)
        {
            ncomp::PunctualLight* component       = pair.first;
            SimpleLight&          simplifiedlight = mSimplifiedlights[pair.second];

            component->UpdateStruct(simplifiedlight);
        }

        mBuffer.StageSection(updateInfo.RenderInfo.GetFrameNumber(), mSimplifiedlights.data(), 0, sizeof(SimpleLight) * mSimplifiedlights.size());

        util::DualBuffer::DeviceBufferState beforeAndAfter{.AccessFlags        = VkAccessFlagBits::VK_ACCESS_SHADER_READ_BIT,
                                                           .PipelineStageFlags = VkPipelineStageFlagBits::VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
                                                           .QueueFamilyIndex   = VK_QUEUE_FAMILY_IGNORED};
        mBuffer.CmdCopyToDevice(updateInfo.RenderInfo.GetFrameNumber(), updateInfo.CmdBuffer, beforeAndAfter, beforeAndAfter);
    }

}  // namespace foray::scene::gcomp