# Synchronization in Foray (and Vulkan)

Vulkan requires the programmer to assure correct synchronization. Any commands submitted via a command buffer are generally allowed to be executed out-of-order. Pipeline barriers are necessary in certain situations to assure data required is in the correct layout and available when the graphics processor accesses it.

## Resources considered Constants during Rendering
The below listed resources are considered constants during rendering, so you are not required to protect access to these with pipeline barriers
* Vertex and Index Buffer
* Scene Textures
* Scene Materials
* Geometry Meta Buffer

## When and what to synchronize
* Sampled images and storage images require pipeline barriers protecting against read/write **before** access (unless they are considered constant). Use the ImageLayoutCache (member of FrameRenderInfo struct) to maintain image layout of images when your barriers transition the layout. The image layout **must** be cached so it can be set in the oldLayout field. If it is not set (left to `VK_IMAGELAYOUT_UNDEFINED`), the driver may **discard all image data on layout transition**.
* Storage and Uniform Buffers require pipeline barriers protecting against read/write **before** access (unless they are considered constant).
