/*
    common/normaltbn.glsl

    Defines methods for calculating normals based on interpolated vertex normals and normalmap readout
*/

#ifndef NORMALTBN_GLSL
#define NORMALTBN_GLSL

mat3 CalculateTBN(in vec3 normal, in vec3 tangent)
{
    vec3 N   = normalize(normal);
    vec3 T   = normalize(tangent);
    vec3 B   = cross(N, T);
    return mat3(T, B, N);
}

/// @brief Calculate normal by incorporating tangent space normalmap output
/// @param normal Interpolated vertex normal in world space
/// @param tangent Interpolated vertex tangent in world space
/// @param normalMap Tangent space normalmap readout
vec3 ApplyNormalMap(in mat3 tbn, in vec3 normalMap)
{
    return tbn * normalize(normalMap * 2.0 - vec3(1.0));
}

#ifdef GLTF_GLSL // Requires MaterialProbe struct definition

/// @brief Calculate normal by incorporating tangent space normalmap output
/// @param normal Interpolated vertex normal in world space
/// @param tangent Interpolated vertex tangent in world space
/// @param probe MaterialProbe containing the Tangent space normalmap probe
vec3 ApplyNormalMap(in mat3 tbn, in MaterialProbe probe)
{
    if(probe.Normal == vec3(0.5f, 0.5f, 1.f))
    {
        return normalize(tbn[2]);
    }
    else
    {
        return tbn * normalize(probe.Normal * 2.0 - vec3(1.0));
    }
}

#endif  // GLTF_GLSL
#endif  // NORMALTBN_GLSL