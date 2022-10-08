#include "foray_denoiserstage.hpp"

namespace foray::stages {
    void DenoiserSynchronisationSemaphore::Create(const core::VkContext* context)
    {
        mContext = context;

#ifdef WIN32
        VkExternalSemaphoreHandleTypeFlagBits handleType = VkExternalSemaphoreHandleTypeFlagBits::VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT_KHR;
#else
        VkExternalSemaphoreHandleTypeFlagBits handleType = VkExternalSemaphoreHandleTypeFlagBits::VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT;
#endif

        VkSemaphoreTypeCreateInfo timelineSemaphoreCi{
            .sType         = VkStructureType::VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
            .semaphoreType = VkSemaphoreType::VK_SEMAPHORE_TYPE_TIMELINE,
            .initialValue  = mValue,
        };

        VkExportSemaphoreCreateInfo exportSemaphoreCi{
            .sType       = VkStructureType::VK_STRUCTURE_TYPE_EXPORT_SEMAPHORE_CREATE_INFO,
            .pNext       = &timelineSemaphoreCi,
            .handleTypes = static_cast<VkExternalSemaphoreHandleTypeFlags>(handleType),
        };

        VkSemaphoreCreateInfo semaphoreCi{.sType = VkStructureType::VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, .pNext = &exportSemaphoreCi, .flags = 0};

        AssertVkResult(vkCreateSemaphore(mContext->Device, &semaphoreCi, nullptr, &mSemaphore));

#ifdef WIN32

#else
        VkSemaphoreGetFdInfoKHR getFdInfo{
                          .sType      = VkStructureType::VK_STRUCTURE_TYPE_SEMAPHORE_GET_FD_INFO_KHR,
                          .semaphore  = mSemaphore,
                          .handleType = handleType,
        };

        AssertVkResult(mContext->DispatchTable.getSemaphoreFdKHR(&getFdInfo, &mHandle));
#endif
    }

    void DenoiserSynchronisationSemaphore::Destroy() 
    {
#ifdef WIN32
#else
    if (mHandle != -1)
    {
        close(mHandle);
        mHandle = -1;
    }
#endif
    if (!!mSemaphore)
    {
        vkDestroySemaphore(mContext->Device, mSemaphore, nullptr);
    }
    }
}  // namespace foray
