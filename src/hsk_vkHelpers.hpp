#pragma once
#include "hsk_basics.hpp"
#include <vulkan/vulkan.h>

namespace hsk{
        
    inline void AssertVkResult(VkResult result)
    {
        if(result != VK_SUCCESS)
        {
            Exception::Throw("VkResult != VK_SUCCESS: ", (int32_t)result); // TODO: might be feasible to convert this to a string value via NAMEOF_ENUM? Just did not want nameof header automatically included by this header...
        }
    }

}