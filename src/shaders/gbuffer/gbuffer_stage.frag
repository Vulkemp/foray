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
layout(location = 6) flat in uint inMeshInstanceId;  // Mesh Instance Id

layout(location = 0) out vec4 outPosition;        // Fragment position in world spcae
layout(location = 1) out vec4 outNormal;          // Fragment normal in world space
layout(location = 2) out vec4 outAlbedo;          // Fragment raw albedo
layout(location = 3) out vec2 outMotion;          // Fragment screenspace motion delta
layout(location = 4) out int outMaterialIndex;    // Material Index
layout(location = 5) out uint outMeshInstanceId;  // Fragment mesh id

#include "bindpoints.glsl"
#include "../common/gltf_pushc.glsl"
#include "../common/materialbuffer.glsl"
#include "../common/normaltbn.glsl"


void main()
{
    outMeshInstanceId = inMeshInstanceId;
    outPosition       = vec4(inWorldPos, 1.0);

    outMaterialIndex = PushConstant.MaterialIndex;

    MaterialBufferObject material = GetMaterialOrFallback(PushConstant.MaterialIndex);

    MaterialProbe probe = ProbeMaterial(material, inUV);

    if (probe.BaseColor.w <= 0) // Alpha coverage discard
    {
        discard;
    }

    outNormal = vec4(ApplyNormalMap(CalculateTBN(inNormal, inTangent), probe), 0.f);

    // Get albedo.
    outAlbedo = probe.BaseColor;

    // Calculate motion delta
    vec2 screenPos     = inDevicePos.xy / inDevicePos.w;
    vec2 old_screenPos = inOldDevicePos.xy / inOldDevicePos.w;
    outMotion          = (old_screenPos - screenPos) * 0.5;
}
