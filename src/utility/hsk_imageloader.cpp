#include "hsk_imageloader.hpp"
#include "hsk_imageloader.inl"


namespace ignore{
    void test() {
        hsk::ImageLoader<VkFormat::VK_FORMAT_R16G16_SFLOAT> loader_0;

        bool result = loader_0.Init("test.exr");

        if (!result){
            return;
        }

        result = loader_0.Load();

        if (!result)
        {
            return;
        }

        loader_0.GetRawData();
    }
}