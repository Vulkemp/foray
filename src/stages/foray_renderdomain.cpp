#include "foray_renderdomain.hpp"
#include "foray_renderstage.hpp"


namespace foray::stages {
    void RenderDomain::InvokeResize(VkExtent2D extent)
    {
        mOnResized.Invoke(extent);
    }
}  // namespace foray::stages
