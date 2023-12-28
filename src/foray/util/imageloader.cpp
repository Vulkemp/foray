#include "imageloader.hpp"
#include "imageloader.inl"
#include "../core/image.hpp"

namespace foray::util::impl {
     void DeleteExrLoaderCache(void* loaderCache) { delete reinterpret_cast<ExrLoaderCache*>(loaderCache); }
}