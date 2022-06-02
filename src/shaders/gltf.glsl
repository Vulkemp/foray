#ifndef MATERIALBUFFER_GLSL
#define MATERIALBUFFER_GLSL

#define MATERIAL_BUFFER_COUNT 64
#define TEXTURE_BUFFER_COUNT 64

struct MaterialBufferObject  // 52 Bytes, aligned to 16 bytes causes size to be padded to a total of 64 bytes
{
    vec4  BaseColorFactor;                // Base Color / Albedo Factor
    float MetallicFactor;                 // Metallic Factor
    vec3  EmissiveFactor;                 // Emissive Factor
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

MaterialBufferObject GetMaterialOrFallback(in int index, in MaterialBufferObject materials[MATERIAL_BUFFER_COUNT])
{
    if(index >= 0)
    {
        return materials[index];
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

MaterialProbe ProbeMaterial(in MaterialBufferObject material, in vec2 uv, in sampler2D textures[TEXTURE_BUFFER_COUNT])
{
    MaterialProbe result;

    // Grab BaseColor / Albedo
    if(material.BaseColorTextureIndex >= 0)
    {
        result.BaseColor = texture(textures[material.BaseColorTextureIndex], uv) * material.BaseColorFactor;
    }
    else
    {
        result.BaseColor = material.BaseColorFactor;
    }

    // Grab Emissive
    if(material.EmissiveTextureIndex >= 0)
    {
        result.EmissiveColor = texture(textures[material.EmissiveTextureIndex], uv).xyz * material.EmissiveFactor;
    }
    else
    {
        result.EmissiveColor = material.EmissiveFactor;
    }

    // Grab Metallic + Roughness
    if(material.MetallicRoughnessTextureIndex >= 0)
    {
        result.MetallicRoughness   = texture(textures[material.MetallicRoughnessTextureIndex], uv).xy;
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
        result.Normal = texture(textures[material.NormalTextureIndex], uv).xyz;
    }
    else
    {
        result.Normal = vec3(0.f, 1.f, 0.f);
    }

    return result;
}

#endif  // MATERIALBUFFER_GLSL
