#include "drawmanager.hpp"
#include "../components/camera.hpp"
#include "../components/meshinstance.hpp"
#include "../components/transform.hpp"
#include "../node.hpp"
#include "../scene.hpp"
#include "../globalcomponents/geometrymanager.hpp"
#include <map>
#include <spdlog/fmt/fmt.h>

namespace foray::scene::gcomp {
    void DrawDirector::InitOrUpdate()
    {
        auto scene = GetScene();
        mGeo       = scene->GetComponent<GeometryStore>();
        std::vector<Node*> meshInstanceNodes;
        scene->FindNodesWithComponent<ncomp::MeshInstance>(meshInstanceNodes);
        std::map<Mesh*, std::vector<ncomp::MeshInstance*>> meshMaps;

        for(auto node : meshInstanceNodes)
        {
            ncomp::MeshInstance* meshInstance = node->GetComponent<ncomp::MeshInstance>();

            auto iter = meshMaps.find(meshInstance->GetMesh());

            if(iter == meshMaps.end())
            {
                meshMaps[meshInstance->GetMesh()] = std::initializer_list<ncomp::MeshInstance*>({meshInstance});
            }
            else
            {
                iter->second.push_back(meshInstance);
            }
        }

        mDrawOps.clear();
        mTotalCount = 0;

        for(auto& mesh : meshMaps)
        {
            DrawOp drawop          = {};
            drawop.Instances       = mesh.second;
            drawop.Target          = mesh.first;
            drawop.TransformOffset = mTotalCount;
            mTotalCount += drawop.Instances.size();
            mDrawOps.push_back(std::move(drawop));
        }

        VkDeviceSize maxcount = (mCurrentTransformBuffer ? mCurrentTransformBuffer->GetDeviceBuffer().GetSize() : 0) / sizeof(glm::mat4);

        if(maxcount < mTotalCount)
        {
            DestroyBuffers();
            CreateBuffers(mTotalCount);
        }
    }

    void DrawDirector::CreateBuffers(size_t transformCount)
    {
        VkDeviceSize size = transformCount * sizeof(glm::mat4);
        size += size / 4;  // Add a bit of extra capacity

        core::ManagedBuffer::CreateInfo ci(VkBufferUsageFlagBits::VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_DST_BIT
                                                            | VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                                        size, VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, 0, "Current Transforms");
        mCurrentTransformBuffer.New(GetContext(), ci);
        ci.Name = "Previous Transforms";
        mPreviousTransformBuffer.New(GetContext(), ci);
    }
    void DrawDirector::DestroyBuffers()
    {
        mCurrentTransformBuffer.Delete();
        mPreviousTransformBuffer.Delete();
    }

    void DrawDirector::Update(SceneUpdateInfo& updateInfo)
    {
        VkCommandBuffer cmdBuffer = updateInfo.CmdBuffer;

        VkDeviceSize bufferSize = sizeof(glm::mat4) * mTotalCount;

        // Transfer previous frames transform buffer state to mPreviousTransformBuffer
        VkBufferMemoryBarrier prevTransformBufferBarrier{.sType               = VkStructureType::VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
                                                         .srcAccessMask       = VkAccessFlagBits::VK_ACCESS_SHADER_READ_BIT,
                                                         .dstAccessMask       = VkAccessFlagBits::VK_ACCESS_TRANSFER_WRITE_BIT,
                                                         .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                                                         .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                                                         .buffer              = mPreviousTransformBuffer->GetBuffer(),
                                                         .offset              = 0,
                                                         .size                = bufferSize};
        VkBufferMemoryBarrier currTransformBufferBarrier{.sType               = VkStructureType::VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
                                                         .srcAccessMask       = VkAccessFlagBits::VK_ACCESS_SHADER_READ_BIT,
                                                         .dstAccessMask       = VkAccessFlagBits::VK_ACCESS_TRANSFER_READ_BIT,
                                                         .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                                                         .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                                                         .buffer              = mPreviousTransformBuffer->GetBuffer(),
                                                         .offset              = 0,
                                                         .size                = bufferSize};

        std::vector<VkBufferMemoryBarrier> memBarriers{prevTransformBufferBarrier, currTransformBufferBarrier};

        vkCmdPipelineBarrier(cmdBuffer, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VkDependencyFlagBits::VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, memBarriers.size(), memBarriers.data(), 0, nullptr);

        VkBufferCopy copyRegion{.srcOffset = 0, .dstOffset = 0, .size = bufferSize};
        vkCmdCopyBuffer(cmdBuffer, mCurrentTransformBuffer->GetDeviceBuffer().GetBuffer(), mPreviousTransformBuffer->GetBuffer(), 1, &copyRegion);

        prevTransformBufferBarrier.srcAccessMask = VkAccessFlagBits::VK_ACCESS_TRANSFER_WRITE_BIT;
        prevTransformBufferBarrier.dstAccessMask = VkAccessFlagBits::VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(cmdBuffer, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TRANSFER_BIT, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
                             VkDependencyFlagBits::VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 1, &prevTransformBufferBarrier, 0, nullptr);

        // Get transform state, upload

        std::vector<glm::mat4> transformStates(mTotalCount);

        for(auto& drawop : mDrawOps)
        {
            for(uint32_t i = 0; i < drawop.Instances.size(); i++)
            {
                auto& transformState = transformStates[drawop.TransformOffset + i];
                auto  transform      = drawop.Instances[i]->GetNode()->GetComponent<ncomp::Transform>();
                transformState       = transform->GetGlobalMatrix();
            }
        }

        mCurrentTransformBuffer->StageSection(updateInfo.RenderInfo.GetFrameNumber(), transformStates.data(), 0, transformStates.size() * sizeof(glm::mat4));

        mCurrentTransformBuffer->CmdCopyToDevice(updateInfo.RenderInfo.GetFrameNumber(), cmdBuffer);
    }

    void DrawDirector::Draw(SceneDrawInfo& drawInfo)
    {

        if(!!mGeo)
        {
            mGeo->CmdBindBuffers(drawInfo.CmdBuffer);
        }

        for(auto& drawop : mDrawOps)
        {
            drawInfo.CmdPushConstant_TransformBufferOffset(drawop.TransformOffset);
            drawop.Target->CmdDrawInstanced(drawInfo, drawop.Instances.size());
        }
    }
}  // namespace foray::scene::gcomp