Rapid Prototyping **F**ramework for **O**pen crossplatform development of Vulkan Hardware **Ray** tracing Demos.
# ⚠️ Notice
* This library is still under heavy development. The Api is still subject to frequent breaking changes.
# Useful Links
* [foray-examples](https://github.com/Vulkemp/foray-examples): Examples demonstrating GBuffer, minimal raytracer, complex raytracer
* [foray-denoising](https://github.com/Vulkemp/foray-denoising): Project demonstrating and comparing denoiser implementations: OptiX, ASVGF, BMFR (WIP)
* [foray_restir](https://github.com/Vulkemp/foray_restir): ReStir demo implementation
# Features
## Quick Setup or Custom Setup
* Inherit from base types to get a very quick and easy start
    * Includes integrated instance, device, window, swapchain setup
    * Renderloop integration to manage application lifetime and event handling
    * Builtin support for in flight frame rendering, may your GPU never be idle
* Utilise the support types manually for a fully customizable setup
## Scene Graph
* Hierarchical scene graph
* Transformations using glm math library
* Component system
* Default component implementations based on glTF format specifications
### Loading from glTF
* The scenegraph is built by loading a glTF file
    * glTFs base material system is well suited for physically based rendering (at the level possible in real time on GPUs). Our Implementation supports `KHR_materials_ior`, `KHR_materials_transmission` and `KHR_materials_volume` extensions to improve it further
    * Support for punctual and direction lights `KHR_lights_punctual`
* Multiple scenes can be merged into one
### Integration with Acceleration Structures
* Meshes will automatically generate a BLAS
* The scenegraph can build a TLAS based on the meshes referenced
* Material and Primitive buffer information is stored in a BLAS Instance `instanceCustomOffset` property
* Runtime TLAS rebuilding for animated scene nodes
### Animation Support
* Animate node transforms (translation, scale, rotation)
* Interpolate in steps, linearly and glTFs cubic splines
## Rasterized GBuffer
The GBuffer provides the basis for denoising
### Outputs
* `Position`: Fragment positions in Worldspace
* `Normals`: Fragment normals in Worldspace (includes normal mapping)
* `Albedo`: Raw surface albedo values
* `Motion`: Screenspace Fragment motion vectors (includes movement of animated objects). Each pixel projects back to its position in the previous frame in UV coordinates with subpixel accuracy.
* `MaterialIdx`: Material index as defined by the material system
* `MeshInstanceIdx`: Mesh Instance Index as defined by the draw system. Every object gets a unique index.
* `LinearZ`: Linear depth information and depth derivative
* `Depth`: Default Vulkan Depth output
## Shader Binding Table and Ray tracing Pipeline Helper classes
* Very little code required for complex shader binding table setup
* Ability to set custom data for shader binding table entries
* Integration into a builder class for Ray tracing Pipeline
## Generalized Denoiser Interface
* Simple interface for integrating denoisers into custom render system
    *Extra handling required for externally synchronized denoisers such as OptiX
## Common Vulkan Helper classes
* VkBuffer wrapper
* VkImage wrapper
* DescriptorSet + Layout wrapper
* ... many more
## Device Benchmarking
* Vulkan QueryPool based for accurate device execution time benchmarking
* Included methods for printing benchmark results in a CSV compatible format or as a ImGui table
## Shader Management
* Shader sources and included files are monitored for changes at runtime, shaders recompiled and reloaded.
* Shader compilation via glslc executable can be parameterized with additional include directories, macro definitions etc.
* SPIR-V binaries compiled are handled individually according to the glslc parameters used for compilation.


Get an overview of the code in the [./src/ directory](./src/)
# Usage
1. Add the library as a submodule to your git repository
```sh
git submodule add https://github.com/Vulkemp/foray
git submodule init
git submodule update
```
2. Add the library directory as a cmake subdirectory
```cmake
add_subdirectory("foray")
```
3. Setup Include Directories and Linking in your target cmake files
```cmake
# Link foray lib
target_link_libraries(
	${PROJECT_NAME}
	PUBLIC foray
)

# Windows requires SDL2 libs linked specifically
if (WIN32)
	target_link_libraries(
		${PROJECT_NAME}
		PUBLIC ${SDL2_LIBRARIES}
	)
endif()


# Configure include directories
target_include_directories(
   	${PROJECT_NAME}
   	PUBLIC "${CMAKE_SOURCE_DIR}/foray/src" # Foray includes
   	PUBLIC "${CMAKE_SOURCE_DIR}/foray/third_party" # Third Party includes
   	PUBLIC ${Vulkan_INCLUDE_DIR}
)
```
# Third Party Dependencies
* [GLM](https://github.com/g-truc/glm): GLSL-like mathematics library (MIT License)
* [ImGui](https://github.com/ocornut/imgui): Immediate Mode GUI for simple user interfaces (MIT License)
* [NameOf](https://github.com/Neargye/nameof): Compiler wizardry providing string representations of enum types etc. for debugging (MIT License)
* [SDL2](https://github.com/libsdl-org/SDL): Cross platform OS interface (ZLib License)
* [spdlog](https://github.com/gabime/spdlog): Formatted logging library (MIT License)
* [tinyexr](https://github.com/syoyo/tinyexr): Loading library for EXR high dynamic range image file format (Custom License)
* [tinygltf](https://github.com/syoyo/tinygltf): glTF loader library (MIT License)
* [vk-bootstrap](https://github.com/charles-lunarg/vk-bootstrap): Utility library for instance creation, device selection and building, queue selection and swapchain creation (MIT License)
* [Vulkan Memory Allocator](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator): Simplified Vulkan memory allocation (MIT License)