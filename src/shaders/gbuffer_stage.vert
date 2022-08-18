#version 450
#extension GL_GOOGLE_include_directive : enable
#extension GL_KHR_vulkan_glsl : enable

layout(location = 0) in vec3 inPos;           // Vertex position in model space
layout(location = 1) in vec3 inNormal;        // Vertex normal
layout(location = 2) in vec3 inTangent;       // Vertex tangent
layout(location = 3) in vec2 inUV;            // UV coordinates

layout(location = 0) out vec3 outWorldPos;             // Vertex position in world space
layout(location = 1) out vec4 outDevicePos;            // Vertex position in normalized device space (current frame)
layout(location = 2) out vec4 outOldDevicePos;         // Vertex position in normalized device space (previous frame)
layout(location = 3) out vec3 outNormal;               // Normal in world space
layout(location = 4) out vec3 outTangent;              // Tangent in world space
layout(location = 5) out vec2 outUV;                   // UV coordinates
layout(location = 6) flat out uint outMeshInstanceId;  // Mesh Instance Id

#define BIND_PUSHC
#include "gltf_pushc.glsl"

#define BIND_CAMERA_UBO 2
#include "camera.glsl"

#define BIND_TRANSFORMBUFFER_CURRENT 3
#define BIND_TRANSFORMBUFFER_PREVIOUS 4
#include "transformbuffer.glsl"

void main()
{
    mat4 ProjMat      = Camera.ProjectionMatrix;
    mat4 ViewMat      = Camera.ViewMatrix;
    mat4 ModelMat     = GetCurrentTransform(PushConstant.TransformBufferOffset + gl_InstanceIndex);
    mat4 ProjMatPrev  = Camera.PreviousProjectionMatrix;
    mat4 ViewMatPrev  = Camera.PreviousViewMatrix;
    mat4 ModelMatPrev = GetPreviousTransform(PushConstant.TransformBufferOffset + gl_InstanceIndex);

    // Get transformations out of the way
    outWorldPos     = (ModelMat * vec4(inPos, 1.f)).xyz;
    outDevicePos    = Camera.ProjectionViewMatrix * ModelMat * vec4(inPos, 1.f);
    gl_Position     = outDevicePos;
    outOldDevicePos = Camera.PreviousProjectionViewMatrix * ModelMatPrev * vec4(inPos, 1.f);

    outUV = inUV;

    // Normal in world space
    mat3 mNormal = transpose(inverse(mat3(ModelMat)));
    outNormal    = mNormal * normalize(inNormal);
    outTangent   = mNormal * normalize(inTangent);

    outMeshInstanceId = PushConstant.TransformBufferOffset + gl_InstanceIndex;
}
