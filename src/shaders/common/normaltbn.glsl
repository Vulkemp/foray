/*
    normaltbn.glsl

    Defines methods for calculating normals based on interpolated vertex normals and normalmap readout
*/

#ifndef NORMALTBN_GLSL
#define NORMALTBN_GLSL

/// @brief Calculate normal
/// @param normal Interpolated vertex normal in world space
/// @param tangent Interpolated vertex tangent in world space
/// @param normalMap Normalmap readout
vec3 ApplyNormalMap(in vec3 normal, in vec3 tangent, in vec3 normalMap)
{
    vec3 N   = normalize(normal);
    vec3 T   = normalize(tangent);
    vec3 B   = cross(N, T);
    mat3 TBN = mat3(T, B, N);
    return TBN * normalize(normalMap * 2.0 - vec3(1.0));
}

#ifdef GLTF_GLSL

vec3 ApplyNormalMap(in vec3 normal, in vec3 tangent, in MaterialProbe probe)
{
    if(probe.Normal == vec3(0.5f, 0.5f, 1.f))
    {
        return normalize(normal);
    }
    else
    {
        vec3 N   = normalize(normal);
        vec3 T   = normalize(tangent);
        vec3 B   = cross(N, T);
        mat3 TBN = mat3(T, B, N);
        return TBN * normalize(probe.Normal * 2.0 - vec3(1.0));
    }
}

#endif  // GLTF_GLSL
#endif  // NORMALTBN_GLSL