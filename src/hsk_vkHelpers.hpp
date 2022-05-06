#pragma once
#include "hsk_basics.hpp"
#include <string_view>
#include <vulkan/vulkan.h>

namespace hsk {

    std::string_view PrintVkResult(VkResult result);

    inline void AssertVkResult(VkResult result)
    {
        if(result != VK_SUCCESS)
        {
            Exception::Throw("VkResult Assertion Failed: VkResult::{}", PrintVkResult(result));
        }
    }

#define HSK_ASSERT_VKRESULT(action)                                                                                                                                                \
    {                                                                                                                                                                              \
        VkResult result = (action);                                                                                                                                                \
        if(result != VK_SUCCESS)                                                                                                                                                   \
        {                                                                                                                                                                          \
            hsk::Exception::Throw("VkResult Assertion Failed: VkResult::{}, call \"{}\"", hsk::PrintVkResult(result), #action);                                                    \
        }                                                                                                                                                                          \
    }

}  // namespace hsk