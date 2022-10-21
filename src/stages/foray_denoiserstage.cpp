#include "foray_denoiserstage.hpp"

namespace foray::stages {
    void DenoiserSynchronisationSemaphore::Create(core::Context* context)
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

        AssertVkResult(mContext->VkbDispatchTable->createSemaphore(&semaphoreCi, nullptr, &mSemaphore));

#ifdef WIN32
        VkSemaphoreGetWin32HandleInfoKHR getWInfo
        {
            .sType = VkStructureType::VK_STRUCTURE_TYPE_SEMAPHORE_GET_WIN32_HANDLE_INFO_KHR,
            .semaphore = mSemaphore,
            .handleType = handleType
        };

        PFN_vkGetSemaphoreWin32HandleKHR getHandleFunc = reinterpret_cast<PFN_vkGetSemaphoreWin32HandleKHR>(vkGetDeviceProcAddr(mContext->Device(), "vkGetSemaphoreWin32HandleKH"
                                                                                                                                                    "R"));

        if(!getHandleFunc)
        {
            Exception::Throw("Unable to resolve vkGetMemoryWin32HandleKHR device proc addr!");
        }

        getHandleFunc(mContext->Device(), &getWInfo, &mHandle);
#else
        VkSemaphoreGetFdInfoKHR               getFdInfo{
                          .sType      = VkStructureType::VK_STRUCTURE_TYPE_SEMAPHORE_GET_FD_INFO_KHR,
                          .semaphore  = mSemaphore,
                          .handleType = handleType,
        };

        AssertVkResult(mContext->VkbDispatchTable->getSemaphoreFdKHR(&getFdInfo, &mHandle));
#endif
    }

    void DenoiserSynchronisationSemaphore::Destroy()
    {
#ifdef WIN32
        if (mHandle != INVALID_HANDLE_VALUE)
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
        }
    }
}  // namespace foray::stages
