#pragma once
#include "exception.hpp"
#include "glm.hpp"
#include "logger.hpp"
#include "vkb.hpp"
#include "vma.hpp"
#include "mem.hpp"

#include "base/base.hpp"
#include "core/core.hpp"
#include "osi/osi.hpp"
#include "scene/scenegraph.hpp"
#include "stages/configurablerasterstage.hpp"
#include "stages/defaultraytracingstage.hpp"
#include "stages/imagetoswapchain.hpp"
#include "stages/imguistage.hpp"

#include "gltf/modelconverter.hpp"

namespace foray::api {
    using DefaultAppBase  = foray::base::DefaultAppBase;
    using FrameRenderInfo = foray::base::FrameRenderInfo;

    using Context = foray::core::Context;

    using ManagedBuffer = foray::core::ManagedBuffer;
    using Image  = foray::core::Image;

    using ConfigurableRasterStage    = foray::stages::ConfigurableRasterStage;
    using ImguiStage                 = foray::stages::ImguiStage;
    using ImageToSwapchainStage      = foray::stages::ImageToSwapchainStage;
    using DefaultRaytracingStageBase = foray::stages::DefaultRaytracingStageBase;
}  // namespace foray::api
