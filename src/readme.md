# Source Code Overview
## Acceleration Structure Abstraction
```
./as
```

* Abstraction for BLAS and TLAS
* Geometry Meta buffer for maintaining meta data for instances written to TLAS and individual BLAS geometries.
## Base classes
```
./base
```
* DefaultAppBase and MinimalAppBase application base classes
* Vulkan Instance, Device, WindowSwapchain context wrappers
* Renderloop implementation
* Rendering structs
## Benchmarking
```
./bench
```
* Benchmark base class for maintaining logs of individual benchmark runs
* Host benchmark implementation
* Device benchmark implementation
## Core Abstractions and Features
```
./core
```
* Abstractions for VkCommandBuffer, VkDescriptorSet, VkBuffer, VkImage, VkShaderModule
* Manager classes for shaders and samplers
* Context struct implementation
## glTF Loader Implementation
```
./gltf
```
* Loader for glTF files writing directly into the scene graph
## Operating System Interface
```
./osi
```
* C++ wrapper based on SDL2
* Window Management
* OS Events
* Input Devices
## Ray Tracing Pipeline Wrapper
```
./rtpipe
```
* Abstraction of shader binding table and Ray Tracing Pipeline
## Scene Graph
```
./scene
```
* Component implementations in `components`
    * Transform
    * Camera and Camera Controller
    * Mesh Instance
    * Punctual Light
* Global Component implementations in `globalcomponents` (scene level singletons)
    * Animation Manager
    * Camera Manager (maintains camera matrices Ubo)
    * Draw Manager (maintains rasterized instanced drawing of mesh instances and model to world transformation buffers)
    * Geometry Manager (vertex and index buffer)
    * Light Manager (punctual lights)
    * Material Manager
    * Texture Manager
    * TLAS Manager
* Animation support
* Entity Component System with event distribution
* Geometry management
* Material management
## Prebuilt shader (includes)
```
./shaders
```
* Include files for binding all buffers provided by other implementations in the framework
* GPU-generated random numbers via xtea and lcg
* Methods for getting geometry and material informations in ray tracing shaders
* Methods for evaluating material reflectance
## Render Stages
```
./stages
```
* RenderStage base class
* RaytracingStage, ComputeStage, RasterizedRenderStage specialized base class
* GBuffer implementation
* Frame Buffer blit stage
* Denoiser stage (denoiser interface)
* ImGui stage
* Comparer stage for comparing frame buffers side by side
## Various Utilities
```
./util
```
* DualBuffer for uploading data to a single device side buffer without conflicts for in flight rendering
* History Image for maintaining a frame buffer from previous rendering
* Image Loader interface combining stbImage and tinyEXR implementations
* Various further wrapper classes
## Common types and includes
```
./
```
* Exceptions and Assertions
* logger (spdlog)
* Including of Vulkan, Vkb, Vma, GLM with configuration