#ifndef COLORSPACE_GLSL
#define COLORSPACE_GLSL

vec3 DecodeGamma(in vec3 rgb)
{
    return pow(rgb, vec3(2.2f));
}

vec3 EncodeGamma(in vec3 rgb)
{
    return pow(rgb, vec3(0.45f));
}

#endif  // COLORSPACE_GLSL