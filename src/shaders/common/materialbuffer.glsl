/*
    materialbuffer.glsl

    Defines layout macros and sampling methods for materials and textures
*/

#include "gltf.glsl"

#ifdef BIND_MATERIAL_BUFFER
#ifndef SET_MATERIAL_BUFFER
#define SET_MATERIAL_BUFFER 0
#endif  // SET_MATERIAL_BUFFER
layout(set = SET_MATERIAL_BUFFER, binding = BIND_MATERIAL_BUFFER, std430) buffer readonly MaterialBuffer
{
    MaterialBufferObject Array[];
}
Materials;
#endif  // BIND_MATERIAL_BUFFER

#ifdef BIND_TEXTURES_ARRAY
#ifndef SET_TEXTURES_ARRAY
#define SET_TEXTURES_ARRAY 0
#endif  // SET_TEXTURES_ARRAY
layout(set = SET_TEXTURES_ARRAY, binding = BIND_TEXTURES_ARRAY) uniform sampler2D Textures[];

vec4 SampleTexture(in int index, in vec2 uv)
{
    return texture(Textures[index], uv);
}

#endif  // BIND_TEXTURES_ARRAY

#ifndef MATERIALBUFFER_GLSL
#define MATERIALBUFFER_GLSL

MaterialBufferObject GetMaterialOrFallback(in int index)
{
    if(index >= 0)
    {
        return Materials.Array[index];
    }
    else
    {
        MaterialBufferObject result;
        result.BaseColorFactor               = vec4(1.f);
        result.MetallicFactor                = 0.5f;
        result.EmissiveFactor                = vec3(0.f);
        result.RoughnessFactor               = 0.5f;
        result.BaseColorTextureIndex         = -1;
        result.MetallicRoughnessTextureIndex = -1;
        result.EmissiveTextureIndex          = -1;
        result.NormalTextureIndex            = -1;
        return result;
    }
}

MaterialProbe ProbeMaterial(in MaterialBufferObject material, in vec2 uv)
{
    MaterialProbe result;

    // Grab BaseColor / Albedo
    if(material.BaseColorTextureIndex >= 0)
    {
        result.BaseColor = SampleTexture(material.BaseColorTextureIndex, uv) * material.BaseColorFactor;
    }
    else
    {
        result.BaseColor = material.BaseColorFactor;
    }

    // Grab Emissive
    if(material.EmissiveTextureIndex >= 0)
    {
        result.EmissiveColor = SampleTexture(material.EmissiveTextureIndex, uv).xyz * material.EmissiveFactor;
    }
    else
    {
        result.EmissiveColor = material.EmissiveFactor;
    }

    // Grab Metallic + Roughness
    if(material.MetallicRoughnessTextureIndex >= 0)
    {
        result.MetallicRoughness = SampleTexture(material.MetallicRoughnessTextureIndex, uv).xy;
        result.MetallicRoughness.x *= material.MetallicFactor;
        result.MetallicRoughness.y *= material.RoughnessFactor;
    }
    else
    {
        result.MetallicRoughness.x = material.MetallicFactor;
        result.MetallicRoughness.y = material.RoughnessFactor;
    }

    // Grab Normal Deviation
    if(material.NormalTextureIndex >= 0)
    {
        result.Normal = SampleTexture(material.NormalTextureIndex, uv).xyz;
    }
    else
    {
        result.Normal = vec3(0.5f, 0.5f, 1.f);
    }

    return result;
}

#endif  // MATERIALBUFFER_GLSL
