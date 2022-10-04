#ifndef COLORSPACE_GLSL
#define COLORSPACE_GLSL

// https://gamedev.stackexchange.com/questions/92015/optimized-linear-to-srgb-glsl

vec3 SrgbToLinear(in vec3 srgb)
{
    bvec3 cutoff = lessThan(srgb, vec3(0.04045));
    vec3 higher = pow((srgb + vec3(0.055))/vec3(1.055), vec3(2.4));
    vec3 lower = srgb/vec3(12.92);

    return mix(higher, lower, cutoff);
}

vec4 SrgbToLinear(in vec4 srgba)
{
    return vec4(SrgbToLinear(srgba.rgb), srgba.a);
}

vec3 LinearToSrgb(in vec3 linear)
{
    bvec3 cutoff = lessThan(linear, vec3(0.0031308));
    vec3 higher = vec3(1.055)*pow(linear, vec3(1.0/2.4)) - vec3(0.055);
    vec3 lower = linear * vec3(12.92);

    return mix(higher, lower, cutoff);
}

vec4 LinearToSrgb(in vec4 linear)
{
    return vec4(LinearToSrgb(linear.rgb), linear.a);
}

#endif  // COLORSPACE_GLSL