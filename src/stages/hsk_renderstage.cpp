#include "hsk_renderstage.hpp"

namespace hsk {
    ManagedImage* RenderStage::GetColorAttachmentByName(const std::string_view name)
    {
        for(auto& attachment : mColorAttachments)
        {
            if(attachment->GetName() == name)
            {
                return attachment.get();
            }
        }
        return nullptr;
    }
}  // namespace hsk