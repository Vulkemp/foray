/*
    rt_common/imageoutput.glsl

    Layout macro for the default output image
*/

#ifdef BIND_OUT_IMAGE
#ifndef SET_OUT_IMAGE
#define SET_OUT_IMAGE 0
#endif  // SET_OUT_IMAGE
#ifndef FORMAT_OUT_IMAGE
#define FORMAT_OUT_IMAGE rgba16f
#endif // FORMAT_OUT_IMAGE
/// @brief Output storage image
layout(set = SET_OUT_IMAGE, binding = BIND_OUT_IMAGE, FORMAT_OUT_IMAGE) uniform writeonly image2D ImageOutput;
#endif  // BIND_OUT_IMAGE
