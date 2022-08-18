#include "hsk_imageloader.hpp"
#include "hsk_imageloader.inl"
#include "../memory/hsk_managedimage.hpp"

namespace hsk::impl {
     void DeleteExrLoaderCache(void* loaderCache) { delete reinterpret_cast<ExrLoaderCache*>(loaderCache); }
}