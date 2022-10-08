#include "foray_imageloader.hpp"
#include "foray_imageloader.inl"
#include "../core/foray_managedimage.hpp"

namespace foray::util::impl {
     void DeleteExrLoaderCache(void* loaderCache) { delete reinterpret_cast<ExrLoaderCache*>(loaderCache); }
}