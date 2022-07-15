#include "hsk_drawdirector.hpp"
#include "../components/hsk_camera.hpp"
#include "../components/hsk_meshinstance.hpp"
#include "../components/hsk_transform.hpp"
#include "../globalcomponents/hsk_geometrystore.hpp"
#include "../hsk_node.hpp"
#include "../hsk_scene.hpp"
#include <map>
#include <spdlog/fmt/fmt.h>

namespace hsk {
    void DrawDirector::InitOrUpdate()
    {
        auto               scene = GetScene();
        std::vector<Node*> meshInstanceNodes;
        scene->FindNodesWithComponent<MeshInstance>(meshInstanceNodes);
        std::map<Mesh*, std::vector<MeshInstance*>> meshMaps;

        for(auto node : meshInstanceNodes)
        {
            MeshInstance* meshInstance = node->GetComponent<MeshInstance>();

            auto iter = meshMaps.find(meshInstance->GetMesh());

            if(iter == meshMaps.end())
            {
                meshMaps[meshInstance->GetMesh()] = std::initializer_list<MeshInstance*>({meshInstance});
            }
            else
            {
                iter->second.push_back(meshInstance);
            }
        }

        mDrawOps.clear();
        uint32_t transformBufferSize = 0;

        for(auto& mesh : meshMaps)
        {
            DrawOp drawop          = {};
            drawop.Instances       = mesh.second;
            drawop.Target          = mesh.first;
            drawop.TransformOffset = transformBufferSize;
            transformBufferSize += drawop.Instances.size();
            mDrawOps.push_back(std::move(drawop));
        }

        for(uint32_t i = 0; i < INFLIGHTFRAMECOUNT; i++)
        {
            if(mFirstSetup)
            {
                mTransformBuffers[i].SetContext(GetContext());
                mTransformBuffers[i].SetName(fmt::format("Transform Buffer #{}", i));
            }
            mTransformBuffers[i].GetVector().resize(transformBufferSize);
            mTransformBuffers[i].InitOrUpdate();
        }
    }

    void DrawDirector::Draw(SceneDrawInfo& drawInfo)
    {
        auto& transformBuffer = mTransformBuffers[drawInfo.RenderInfo.GetFrameNumber()];
        auto& transformVector = transformBuffer.GetVector();

        for(auto& drawop : mDrawOps)
        {
            for(uint32_t i = 0; i < drawop.Instances.size(); i++)
            {
                auto& transformState               = transformVector[drawop.TransformOffset + i];
                transformState.PreviousWorldMatrix = transformState.CurrentWorldMatrix;
                transformState.CurrentWorldMatrix  = drawop.Instances[i]->GetNode()->GetComponent<Transform>()->GetGlobalMatrix();
            }
        }

        transformBuffer.InitOrUpdate();

        for(auto& drawop : mDrawOps)
        {
            drawInfo.CmdPushConstant(drawop.TransformOffset);
            drawop.Target->CmdDrawInstanced(drawInfo.RenderInfo.GetCommandBuffer(), drawInfo.CurrentlyBoundGeoBuffers, drawop.Instances.size());
        }
    }

    std::shared_ptr<DescriptorSetHelper::DescriptorInfo> DrawDirector::MakeDescriptorInfos(VkShaderStageFlags shaderStage)
    {
        auto descriptorInfo = std::make_shared<DescriptorSetHelper::DescriptorInfo>();
        descriptorInfo->Init(VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, shaderStage);

        for(uint32_t i = 0; i < INFLIGHTFRAMECOUNT; i++)
        {
            mTransformBuffers[i].GetBuffer().FillVkDescriptorBufferInfo(mBufferInfos[i].data());
            descriptorInfo->AddDescriptorSet(mBufferInfos + i);
        }
        return descriptorInfo;
    }
}  // namespace hsk