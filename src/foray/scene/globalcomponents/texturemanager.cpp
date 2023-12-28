#include "texturemanager.hpp"
#include "../../vulkan.hpp"
#include "../../util/hash.hpp"
#include <functional>
#include <spdlog/fmt/fmt.h>

namespace foray::scene::gcomp {
    void TextureManager::Destroy()
    {
        mTextures.clear();
    }

    std::vector<vk::DescriptorImageInfo> TextureManager::GetDescriptorInfos(vk::ImageLayout layout)
    {
        std::vector<vk::DescriptorImageInfo> imageInfos(mTextures.size());
        for(size_t i = 0; i < mTextures.size(); i++)
        {
            imageInfos[i] = mTextures[i].GetDescriptorImageInfo(layout);
        }
        return imageInfos;
    }


    void TextureManager::Texture::CreateImage(core::Context* context, const core::Image::CreateInfo& ci)
    {
        mImage.New(context, ci);
        mImageView.New(mImage.Get());
    }

}  // namespace foray::scene::gcomp