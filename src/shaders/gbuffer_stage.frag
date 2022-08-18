#version 450
#extension GL_GOOGLE_include_directive : enable
#extension GL_KHR_vulkan_glsl : enable
#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) in vec3 inWorldPos;             // Vertex position in world space
layout(location = 1) in vec4 inDevicePos;            // Vertex position in normalized device space (current frame)
layout(location = 2) in vec4 inOldDevicePos;         // Vertex position in normalized device space (previous frame)
layout(location = 3) in vec3 inNormal;               // Normal in world space
layout(location = 4) in vec3 inTangent;              // Tangent in world space
layout(location = 5) in vec2 inUV;                   // UV coordinates
layout(location = 6) flat in int inMaterialIndex;    // Material Index
layout(location = 7) flat in uint inMeshInstanceId;  // Mesh Instance Id

layout(location = 0) out vec4 outPosition;        // Fragment position in world spcae
layout(location = 1) out vec4 outNormal;          // Fragment normal in world space
layout(location = 2) out vec4 outAlbedo;          // Fragment raw albedo
layout(location = 3) out vec2 outMotion;          // Fragment screenspace motion delta
layout(location = 4) out int outMaterialIndex;    // Material Index
layout(location = 5) out uint outMeshInstanceId;  // Fragment mesh id


#define BIND_INSTANCE_PUSHC
#include "gltf_pushc.glsl"

#define BIND_MATERIAL_BUFFER 0
#define BIND_TEXTURES_BUFFER 1
#include "materialbuffer.glsl"


void main()
{
    outMeshInstanceId = inMeshInstanceId;
    outPosition       = vec4(inWorldPos, 1.0);

    MaterialBufferObject material = GetMaterialOrFallback(inMaterialIndex);

    MaterialProbe probe = ProbeMaterial(material, inUV);

    // Calculate normal in tangent space
    if(probe.Normal == vec3(0.f, 1.f, 0.f))
    {
        outNormal = vec4(normalize(inNormal), 0.0);
    }
    else
    {
        vec3 N     = normalize(inNormal);
        vec3 T     = normalize(inTangent);
        vec3 B     = cross(N, T);
        mat3 TBN   = mat3(T, B, N);
        vec3 tnorm = TBN * normalize(probe.Normal * 2.0 - vec3(1.0));
        outNormal  = vec4(tnorm, 0.0);
    }

    // Get albedo.
    outAlbedo = probe.BaseColor;

    // Calculate motion delta
    vec2 screenPos     = inDevicePos.xy / inDevicePos.w;
    vec2 old_screenPos = inOldDevicePos.xy / inOldDevicePos.w;
    outMotion          = (old_screenPos - screenPos) * 0.5;

    outMaterialIndex = inMaterialIndex;
}
