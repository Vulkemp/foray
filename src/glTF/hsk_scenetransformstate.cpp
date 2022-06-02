#include "hsk_scenetransformstate.hpp"

namespace hsk {
    void SceneTransformState::InitOrUpdate(std::optional<BufferSection> section)
    {
        mBuffer.SetContext(Context());
        mBuffer.InitOrUpdate(section);
    }

}  // namespace hsk