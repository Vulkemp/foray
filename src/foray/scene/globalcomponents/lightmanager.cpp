#include "lightmanager.hpp"
#include "../node.hpp"
#include "../scene.hpp"

namespace foray::scene::gcomp {
    void LightManager::CreateOrUpdate()
    {
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

        size_t align = alignof(SimpleLight);

        VkDeviceSize bufferSize = align + sizeof(SimpleLight) * mSimplifiedlights.size();

        core::ManagedBuffer::CreateInfo ci(VkBufferUsageFlagBits::VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_DST_BIT, bufferSize,
                                           VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, 0, "Simplifiedlights");
        mBuffer.New(GetContext(), ci);

        uint32_t count = mSimplifiedlights.size();
        mBuffer->StageSection(0, &count, 0, sizeof(count));

        if(count > 0)
        {
            mBuffer->StageSection(0, mSimplifiedlights.data(), align, sizeof(SimpleLight) * count);
        }

        core::HostSyncCommandBuffer cmdBuf(GetContext());
        cmdBuf.Begin();

        mBuffer->CmdCopyToDevice(0, cmdBuf);

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

        if(mSimplifiedlights.size() > 0)
        {
            mBuffer->StageSection(updateInfo.RenderInfo.GetFrameNumber(), mSimplifiedlights.data(), alignof(SimpleLight), sizeof(SimpleLight) * mSimplifiedlights.size());

            mBuffer->CmdCopyToDevice(updateInfo.RenderInfo.GetFrameNumber(), updateInfo.CmdBuffer);
        }
    }

}  // namespace foray::scene::gcomp
