#include "hsk_imageloader.hpp"
#include "hsk_imageloader.inl"
#include "../memory/hsk_managedimage.hpp"


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

        hsk::ImageLoader<VkFormat::VK_FORMAT_A2R10G10B10_UINT_PACK32> loader_1;

        result = loader_1.Init("test.exr");

        if (!result){
            return;
        }

        result = loader_1.Load();

        if (!result)
        {
            return;
        }

        loader_1.GetRawData();

        hsk::ManagedImage image;
        hsk::ManagedImage::CreateInfo ci;
        ci.AllocCI.usage = VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
        

        loader_0.InitManagedImage(nullptr, &image, ci);
    }
}