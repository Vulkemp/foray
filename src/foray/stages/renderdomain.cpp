#include "renderdomain.hpp"
#include "renderstage.hpp"


namespace foray::stages {
    void RenderDomain::InvokeResize(VkExtent2D extent)
    {
        mOnResized.Invoke(extent);
    }
}  // namespace foray::stages
