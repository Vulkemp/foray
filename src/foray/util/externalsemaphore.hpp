#pragma once
#include "../core/managedresource.hpp"

namespace foray::util {

    /// @brief Timeline Semaphore exposing handles for synchronization with other Apis (e.g. Cuda for OptiX denoising)
    class ExternalSemaphore : public core::VulkanResource<VkObjectType::VK_OBJECT_TYPE_SEMAPHORE>
    {
      public:
        ExternalSemaphore(core::Context* context);
        ~ExternalSemaphore();
        inline virtual bool Exists() const override { return !!mSemaphore; }

        FORAY_GETTER_V(Semaphore)
        FORAY_GETTER_V(Handle)

        inline operator VkSemaphore() const { return mSemaphore; }

      protected:
        core::Context* mContext   = nullptr;
        VkSemaphore    mSemaphore = nullptr;
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
        HANDLE mHandle = INVALID_HANDLE_VALUE;
#else
        int mHandle = -1;
#endif
    };

}  // namespace foray::util
