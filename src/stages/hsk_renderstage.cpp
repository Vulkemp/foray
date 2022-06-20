#include "hsk_renderstage.hpp"

namespace hsk {
    ManagedImage* RenderStage::GetColorAttachmentByName(const std::string_view name, bool noThrow)
    {
        for(auto& attachment : mColorAttachments)
        {
            if(attachment->GetName() == name)
            {
                return attachment;
            }
        }
        if(!noThrow)
        {
            throw Exception(std::string("Failed to get color attachment with name: ") + std::string(name));
        }
        return nullptr;
    }
}  // namespace hsk