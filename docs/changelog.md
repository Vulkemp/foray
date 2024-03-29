# Changelog

Documents major and minor changes, patches only if critical fixes were applied.

## Version 1.0.7
* Updated vk-bootstrap to preserve compatibility with newer Vulkan Header versions
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
