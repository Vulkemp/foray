#include "foray_externalsemaphore.hpp"
#include "../core/foray_context.hpp"

namespace foray::util
{
    void ExternalSemaphore::Create(core::Context* context)
    {
        mContext = context;

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
        VkExternalSemaphoreHandleTypeFlagBits handleType = VkExternalSemaphoreHandleTypeFlagBits::VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT_KHR;
#else
        VkExternalSemaphoreHandleTypeFlagBits handleType = VkExternalSemaphoreHandleTypeFlagBits::VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT;
#endif

        VkSemaphoreTypeCreateInfo timelineSemaphoreCi{
            .sType         = VkStructureType::VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
            .semaphoreType = VkSemaphoreType::VK_SEMAPHORE_TYPE_TIMELINE,
            .initialValue  = 0,
        };

        VkExportSemaphoreCreateInfo exportSemaphoreCi{
            .sType       = VkStructureType::VK_STRUCTURE_TYPE_EXPORT_SEMAPHORE_CREATE_INFO,
            .pNext       = &timelineSemaphoreCi,
            .handleTypes = static_cast<VkExternalSemaphoreHandleTypeFlags>(handleType),
        };

        VkSemaphoreCreateInfo semaphoreCi{.sType = VkStructureType::VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, .pNext = &exportSemaphoreCi, .flags = 0};

        AssertVkResult(mContext->VkbDispatchTable->createSemaphore(&semaphoreCi, nullptr, &mSemaphore));

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
        VkSemaphoreGetWin32HandleInfoKHR getWInfo{
            .sType = VkStructureType::VK_STRUCTURE_TYPE_SEMAPHORE_GET_WIN32_HANDLE_INFO_KHR, .semaphore = mSemaphore, .handleType = handleType};

        mContext->VkbDispatchTable->getSemaphoreWin32HandleKHR(&getWInfo, &mHandle);
#else
        VkSemaphoreGetFdInfoKHR               getFdInfo{
                          .sType      = VkStructureType::VK_STRUCTURE_TYPE_SEMAPHORE_GET_FD_INFO_KHR,
                          .semaphore  = mSemaphore,
                          .handleType = handleType,
        };

        AssertVkResult(mContext->VkbDispatchTable->getSemaphoreFdKHR(&getFdInfo, &mHandle));
#endif
    }

    void ExternalSemaphore::Destroy()
    {
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
        if(mHandle != INVALID_HANDLE_VALUE)
        {
            CloseHandle(mHandle);
            mHandle = INVALID_HANDLE_VALUE;
        }
#else
        if(mHandle != -1)
        {
            close(mHandle);
            mHandle = -1;
        }
#endif
        if(!!mSemaphore)
        {
            mContext->VkbDispatchTable->destroySemaphore(mSemaphore, nullptr);
            mSemaphore = nullptr;
        }
    }    
} // namespace foray::util
