#include "foray_aabbmanager.hpp"
#include "../components/foray_aabb.hpp"
#include "../components/foray_transform.hpp"
#include "../components/foray_meshinstance.hpp"
#include "../foray_node.hpp"
#include "../foray_scene.hpp"

namespace foray::scene::gcomp {
    void AabbManager::CompileAabbs()
    {
        std::vector<Node*> nodes;
        GetScene()->FindNodesWithComponent<ncomp::MeshInstance>(nodes);

        glm::vec3 min = glm::vec3(10000000000);
        glm::vec3 max = glm::vec3(-10000000000);

        for(Node* node : nodes)
        {
            ncomp::Transform* transform = node->GetTransform();

            if(!transform->GetStatic())
            {
                continue;  // Node is animated, cannot factor into static scene AABB
            }

            ncomp::AxisAlignedBoundingBox* aabb = node->GetComponent<ncomp::AxisAlignedBoundingBox>();
            if(!aabb)
            {
                aabb = node->MakeComponent<ncomp::AxisAlignedBoundingBox>();
            }

            aabb->CompileAABB();

            const glm::vec3& nodeMin = aabb->GetMinBounds();
            const glm::vec3& nodeMax = aabb->GetMaxBounds();

            min = glm::vec3(std::min(nodeMin.x, min.x), std::min(nodeMin.y, min.y), std::min(nodeMin.z, min.z));
            max = glm::vec3(std::max(nodeMax.x, max.x), std::max(nodeMax.y, max.y), std::max(nodeMax.z, max.z));
        }

        if(min.x > max.x || min.y > max.y || min.z > max.z)
        {
            mMinBounds = glm::vec3(0);
            mMaxBounds = glm::vec3(0);
            mVolume    = 0;
        }
        else
        {
            mMinBounds     = min;
            mMaxBounds     = max;
            glm::vec3 diff = max - min;
            mVolume        = glm::dot(diff, diff);
        }
    }
}  // namespace foray::scene::gcomp