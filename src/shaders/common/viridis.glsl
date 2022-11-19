#ifndef VIRIDIS_GLSL
#define VIRIDIS_GLSL
// https://www.shadertoy.com/view/WlfXRN
// fitting polynomials to matplotlib colormaps
//
// License CC0 (public domain) 
//   https://creativecommons.org/share-your-work/public-domain/cc0/
//
// feel free to use these in your own work!
//
// similar to https://www.shadertoy.com/view/XtGGzG but with a couple small differences:
//
//  - use degree 6 instead of degree 5 polynomials
//  - use nested horner representation for polynomials
//  - polynomials were fitted to minimize maximum error (as opposed to least squares)
//
// data fitted from https://github.com/BIDS/colormap/blob/master/colormaps.py
// (which is licensed CC0)


vec3 viridis(float t) {

    const vec3 c0 = vec3(0.2777273272234177, 0.005407344544966578, 0.3340998053353061);
    const vec3 c1 = vec3(0.1050930431085774, 1.404613529898575, 1.384590162594685);
    const vec3 c2 = vec3(-0.3308618287255563, 0.214847559468213, 0.09509516302823659);
    const vec3 c3 = vec3(-4.634230498983486, -5.799100973351585, -19.33244095627987);
    const vec3 c4 = vec3(6.228269936347081, 14.17993336680509, 56.69055260068105);
    const vec3 c5 = vec3(4.776384997670288, -13.74514537774601, -65.35303263337234);
    const vec3 c6 = vec3(-5.435455855934631, 4.645852612178535, 26.3124352495832);

    return c0+t*(c1+t*(c2+t*(c3+t*(c4+t*(c5+t*c6)))));
}

#endif // VIRIDIS_GLSL