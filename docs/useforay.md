# Notes on how to build upon Foray

## Base Classes

While most classes in foray can be extended via inheritance, there are some types which are specifically made to be inherited from.
* The type name ends with `Base`, example foray::base::DefaultAppBase, foray::stages::DefaultRaytracingStageBase
* Functions which most users are expected to override begin with `Api`, example `ApiInit`, `ApiCreateRtPipeline`, ...
* Functions which must be overridden may be abstract

|Type|Description|
|-|-|
|foray::base::DefaultAppBase|Intended as base class for demo applications.|
|foray::base::MinimalAppBase|Base providing bare minimum of functionality (app lifetime, event handling, vulkan instance management).|
|foray::stages::ComputeStageBase|Base class for compute shaders.|
|foray::stages::DefaultRaytracingStageBase|Raytracing Stage base requiring only rt pipeline setup for a lot of functionality.|
|foray::stages::MinimalRaytracingStageBase|Raytracing Stage providing virtual methods well integrated into the RenderStage interface, but no functionality.|

## Context
The foray::core::Context type is one of the most used types throughout the project. Its purpose is to combine commonly passed initialization and state information into one object. The object itself only represents a loan on its members. This means for a complex setup, the program may maintain multiple sets of the context object. For simple setups, the Context object in the foray::base::DefaultAppBase::mContext (maintained automatically by the DefaultAppBase) is likely the only instance required.

## Other Important Types

|Type|Description|
|-|-|
|foray::base::FrameRenderInfo|Context used for render processes. This object is rebuilt for every frame.|
|foray::core::ManagedBuffer|Wraps allocation and lifetime functionality of a VkBuffer.|
|foray::core::ManagedImage|Wraps allocation and lifetime functionality of VkImage.|
|foray::stages::GBufferStage|Utilizes rasterization to render a GBuffer output.|
|foray::stages::ImageToSwapchainStage|The only purpose of this class is to copy the image onto the swapchain.|
|foray::stages::ImguiStage|Renders the imgui menu on top of an existing image or the swapchain.|
