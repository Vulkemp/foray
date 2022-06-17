#ifndef GLTF_GLSL
#define GLTF_GLSL

struct MaterialBufferObject  // 52 Bytes, aligned to 16 bytes causes size to be padded to a total of 64 bytes
{
    vec4  BaseColorFactor;                // Base Color / Albedo Factor
    vec3  EmissiveFactor;                 // Emissive Factor
    float MetallicFactor;                 // Metallic Factor
    float RoughnessFactor;                // Roughness Factor
    int   BaseColorTextureIndex;          // Texture Index for BaseColor
    int   MetallicRoughnessTextureIndex;  // Texture Index for MetallicRoughness
    int   EmissiveTextureIndex;           // Texture Index for Emissive
    int   NormalTextureIndex;             // Texture Index for Normal
};

struct MaterialProbe
{
    vec4 BaseColor;
    vec3 EmissiveColor;
    vec2 MetallicRoughness;
    vec3 Normal;
};

#endif  // GLTF_GLSL
