# Changelog

Documents major and minor changes, patches only if critical fixes were applied.

## Version dev-2.0.0
* Drastic API changes to achieve a more congruent, modern feel developing with foray
    * All types now base themselves on the concepts of [RAII](https://en.cppreference.com/w/cpp/language/raii)
        * Makes it much less likely that a destruction is omited
        * Makes it much more clear when an object exists
    * Introduced `Heap` and `Local` templates
        * Maintain state of RAII objects which aren't necessarily bound with the parent scope consistently
        * `Heap` loosely based on `std::unique_ptr`, but allows moving pointers from one to another
        * `Local` loosely based on `std::optional`
        * Both share the same API to allow swapping implementations around with minimal changes needed
        * Come with their own set of `FORAY_GETTER` macros defining accessors to the underlying pointer directly
    * `event` API introduces a global method of passing events from delegates to receivers
    * Rename all files to make including files more conventional and easier to read
        * Removed `foray_` prefix, moved files below `foray` directory instead: `#include <bench/foray_logsink.hpp>` becomes `#include <foray/bench/logsink.hpp>`
        * Renamed some headers to better represent their function and contents
            * `env.hpp` → `path.hpp`
    * Separated distribution of `foray::osi` events and `SDLEvent` events. Drastic changes to `foray::osi::OsManager` needed.
        * This was necessary, because third party libs like ImGui rely on more `SDLEvent` types than handled by foray::osi.
    * Less rigid implementation of Descriptor-related wrappers
        * new RAII `DescriptorSetLayout`, `DescriptorPool`, `DescriptorSet` wrappers
        * Reimplementation of previous `DescriptorSet` class based on those wrappers
    * Rewritten `bench` module
        * Function purely on deltas - timestamps were not used in practice
        * Event based API for handling benchmark repetitions. Sinks can log to console, display an imgui table or write to files directly
        * Use a `StringSet` utility class to store interval-id strings for the duration of the benchmark
* New features
    * RenderStage interface now is bound to a RenderDomain, indicating the size of render targets
        * Makes it more feasible to use renderstages for secondary roles, such as 
            * shadow map rendering
            * rendering parts of the pipeline at lower resolution
        * Size changes propagated by events, called in a per-stage defined order to assure dependencies can be resolved properly
* Some fixes to existing functionality
    * Fix Crs::Template::LinearizedDepth clear value
    * Ray tracing features can be fully disabled to allow compatibility of some projects with non-rt hardware
* TODO Features
    * Tonemapping
    * Automatic exposure
    * Implementation of [NRD (denoisers)](https://github.com/NVIDIAGameWorks/RayTracingDenoiser)
    * TAA (temporal anti-aliasing)
    * Port existing implementations to foray2.0
        * Restir DI
        * Spatial Irradiance Cache
        * A-SVGF denoiser
    * Add additional implementations
        * Restir GI
        * Functioning BMFR denoiser
## Version 1.1.0
* Added configurable raster stage
* Define FORAY_SHADER_DIR when compiling foray
## Version 1.0.6
* Fix const-correctness on foray::core::ManagedImage::GetSampleCount()
* Add option to disable some RT calls
* Improve Device Selection Process
    * If multiple suitable devices are detected, the user can choose via a console prompt
    * Log name of selected device
* Fix: Material Evaluation on RDNA2 was broken, now works.
## Version 1.0.5
* Fix: shaderc optimization option did not affect compile hash and was not configured on windows
* Remove 'GL_KHR_vulkan_glsl' from all shader files
* Enable CMAKE option for exception catch mode
* Fix glslc arguments in release mode
## Version 1.0.4
* More robust windows host environment detection at compile time
## Version 1.0.3
* Fixed windows SDL locate error
## Version 1.0.1, 1.0.2
* Documentation updates
## Version 1.0
* Initial Publication

# Versioning
## Major
* API may be subject to breaking changes between major versions
## Minor
* API is guaranteed compatible
* Features may be added
## Patch
* Fully compatible, fixes and documentation updates only
