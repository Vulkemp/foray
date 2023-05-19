#include "aabb.hpp"
#include "../mesh.hpp"
#include "../node.hpp"
#include "../globalcomponents/geometrymanager.hpp"
#include "meshinstance.hpp"
#include "transform.hpp"

namespace foray::scene::ncomp {
    void AxisAlignedBoundingBox::CompileAABB()
    {
        Transform*            transform     = GetNode()->GetTransform();
        MeshInstance*         meshInstance  = GetNode()->GetComponent<MeshInstance>();
        gcomp::GeometryStore* geometryStore = GetGlobals()->GetComponent<gcomp::GeometryStore>();

        if(!transform || !meshInstance || !geometryStore)
        {
            SetAABB(glm::vec3(), glm::vec3());
            return;
        }

        Mesh*     mesh         = meshInstance->GetMesh();
        glm::mat4 modelToWorld = transform->GetGlobalMatrix();

        if(!mesh)
        {
            SetAABB(glm::vec3(), glm::vec3());
            return;
        }

        glm::vec3 min = glm::vec3(10000000000);
        glm::vec3 max = glm::vec3(-10000000000);

        const std::vector<Vertex>&   verticesBuf = geometryStore->GetVertices();
        const std::vector<uint32_t>& indicesBuf  = geometryStore->GetIndices();

        for(const Primitive& primitive : mesh->GetPrimitives())
        {
            uint32_t indicesStart = primitive.First;
            uint32_t indicesCount = primitive.VertexOrIndexCount;
            for(uint32_t index = indicesStart; index < indicesStart + indicesCount; index++)
            {
                const Vertex& vertex    = verticesBuf[indicesBuf[index]];
                glm::vec4     vertexPos = glm::vec4(vertex.Pos, 1);
                vertexPos               = modelToWorld * vertexPos;
                min                     = glm::vec3(std::min(vertexPos.x, min.x), std::min(vertexPos.y, min.y), std::min(vertexPos.z, min.z));
                max                     = glm::vec3(std::max(vertexPos.x, max.x), std::max(vertexPos.y, max.y), std::max(vertexPos.z, max.z));
            }
        }

        SetAABB(min, max);
    }
    void AxisAlignedBoundingBox::SetAABB(const glm::vec3& minBounds, const glm::vec3& maxBounds)
    {
        mMinBounds = minBounds;
        mMaxBounds = maxBounds;
        mVolume    = 1;
        for(int32_t componentIdx = 0; componentIdx < 3; componentIdx++)
        {
            if(mMinBounds[componentIdx] > mMaxBounds[componentIdx])
            {
                mMinBounds[componentIdx] = mMaxBounds[componentIdx];
            }
            mVolume *= (mMaxBounds[componentIdx] - mMinBounds[componentIdx]);
        }
    }
}  // namespace foray::scene::ncomp