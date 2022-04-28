#include "hsk_boundingBox.hpp"

namespace hsk {

    BoundingBox::BoundingBox(){};

    BoundingBox::BoundingBox(glm::vec3 min, glm::vec3 max) : mMin(min), mMax(max){};

    BoundingBox BoundingBox::getAABB(glm::mat4 m)
    {
        glm::vec3 min = glm::vec3(m[3]);
        glm::vec3 max = min;
        glm::vec3 v0, v1;

        glm::vec3 right = glm::vec3(m[0]);

        v0 = right * this->mMin.x;
        v1 = right * this->mMax.x;
        min += glm::min(v0, v1);
        max += glm::max(v0, v1);

        glm::vec3 up = glm::vec3(m[1]);

        v0 = up * this->mMin.y;
        v1 = up * this->mMax.y;
        min += glm::min(v0, v1);
        max += glm::max(v0, v1);

        glm::vec3 back = glm::vec3(m[2]);

        v0 = back * this->mMin.z;
        v1 = back * this->mMax.z;
        min += glm::min(v0, v1);
        max += glm::max(v0, v1);

        return BoundingBox(min, max);
    }


}  // namespace hsk