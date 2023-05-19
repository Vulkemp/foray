#include "imageloader.hpp"
#include "imageloader.inl"
#include "../core/managedimage.hpp"

namespace foray::util::impl {
     void DeleteExrLoaderCache(void* loaderCache) { delete reinterpret_cast<ExrLoaderCache*>(loaderCache); }
}