#pragma once
#include "foray_exception.hpp"
#include "foray_glm.hpp"
#include "foray_logger.hpp"
#include "foray_vkb.hpp"
#include "foray_vma.hpp"
#include "foray_mem.hpp"

#include "base/foray_base.hpp"
#include "core/foray_core.hpp"
#include "osi/foray_osi.hpp"
#include "scene/foray_scenegraph.hpp"
#include "stages/foray_configurablerasterstage.hpp"
#include "stages/foray_defaultraytracingstage.hpp"
#include "stages/foray_imagetoswapchain.hpp"
#include "stages/foray_imguistage.hpp"

#include "gltf/foray_modelconverter.hpp"

namespace foray::api {
    using DefaultAppBase  = foray::base::DefaultAppBase;
    using FrameRenderInfo = foray::base::FrameRenderInfo;

    using Context = foray::core::Context;

    using ManagedBuffer = foray::core::ManagedBuffer;
    using ManagedImage  = foray::core::ManagedImage;

    using ConfigurableRasterStage    = foray::stages::ConfigurableRasterStage;
    using ImguiStage                 = foray::stages::ImguiStage;
    using ImageToSwapchainStage      = foray::stages::ImageToSwapchainStage;
    using DefaultRaytracingStageBase = foray::stages::DefaultRaytracingStageBase;
}  // namespace foray::api
