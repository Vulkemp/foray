#include "foray_renderstage.hpp"

namespace foray::stages {
    std::vector<core::ManagedImage*> RenderStage::GetImageOutputs()
    {
        std::vector<core::ManagedImage*> result;
        result.reserve(mImageOutputs.size());
        for (auto& output : mImageOutputs)
        {
            result.push_back(output.second);
        }
        return result;
    }
    core::ManagedImage* RenderStage::GetImageOutput(const std::string_view name, bool noThrow)
    {
        std::string namecopy(name);
        auto iter = mImageOutputs.find(namecopy);
        if (iter != mImageOutputs.end())
        {
            return iter->second;
        }
        if(!noThrow)
        {
            FORAY_THROWFMT("Failed to get color attachment with name: {}", name)
        }
        return nullptr;
    }
}  // namespace foray