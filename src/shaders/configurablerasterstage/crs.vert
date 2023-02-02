#version 450
#extension GL_GOOGLE_include_directive : enable

layout(location = 0) in vec3 inPos;           // Vertex position in model space
layout(location = 1) in vec3 inNormal;        // Vertex normal
layout(location = 2) in vec3 inTangent;       // Vertex tangent
layout(location = 3) in vec2 inUV;            // UV coordinates

#define INTERFACEMODE out
#include "shaderinterface.glsl"

#include "bindpoints.glsl"
#include "../common/gltf_pushc.glsl"
#include "../common/camera.glsl"
#include "../common/transformbuffer.glsl"

void main()
{
    mat4 ModelMat     = GetCurrentTransform(PushConstant.TransformBufferOffset + gl_InstanceIndex);

#if(INTERFACE_WORLDPOSOLD || INTERFACE_DEVICEPOSOLD)
    mat4 ModelMatPrev = GetPreviousTransform(PushConstant.TransformBufferOffset + gl_InstanceIndex);
#endif

    // Get transformations out of the way
#if INTERFACE_WORLDPOS
    WorldPos     = (ModelMat * vec4(inPos, 1.f)).xyz;
#endif
#if INTERFACE_WORLDPOSOLD
    WorldPosOld     = (ModelMatPrev * vec4(inPos, 1.f)).xyz;
#endif
#ifndef INTERFACE_DEVICEPOS
    vec4 DevicePos;
#endif
    DevicePos    = Camera.ProjectionViewMatrix * ModelMat * vec4(inPos, 1.f);
    gl_Position     = DevicePos;
#if INTERFACE_DEVICEPOSOLD
    DevicePosOld = Camera.PreviousProjectionViewMatrix * ModelMatPrev * vec4(inPos, 1.f);
#endif

#if INTERFACE_UV
    UV = inUV;
#endif

    // Normal in world space
#if(INTERFACE_NORMAL || INTERFACE_TANGENT)
    mat3 mNormal = transpose(inverse(mat3(ModelMat)));
#endif
#if INTERFACE_NORMAL
    Normal    = mNormal * inNormal;
#endif
#if INTERFACE_TANGENT
    Tangent   = mNormal * inTangent;
#endif

#if INTERFACE_MESHID
    MeshInstanceId = PushConstant.TransformBufferOffset + gl_InstanceIndex;
#endif
}
