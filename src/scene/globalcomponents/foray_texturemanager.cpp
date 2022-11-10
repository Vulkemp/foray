#include "foray_texturemanager.hpp"
#include "../../foray_vulkan.hpp"
#include "../../util/foray_hash.hpp"
#include <functional>
#include <spdlog/fmt/fmt.h>

namespace foray::scene::gcomp {
    void TextureManager::Destroy()
    {
        mTextures.clear();
    }

    std::vector<VkDescriptorImageInfo> TextureManager::GetDescriptorInfos(VkImageLayout layout)
    {
        std::vector<VkDescriptorImageInfo> imageInfos(mTextures.size());
        for(size_t i = 0; i < mTextures.size(); i++)
        {
            imageInfos[i] = mTextures[i].GetDescriptorImageInfo(layout);
        }
        return imageInfos;
    }


}  // namespace foray::scene