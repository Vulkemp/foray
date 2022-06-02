#include "hsk_scenetransformstate.hpp"

namespace hsk {
    void SceneTransformState::InitOrUpdate(std::optional<BufferSection> section)
    {
        mBuffer.InitOrUpdate(section);
    }

}  // namespace hsk