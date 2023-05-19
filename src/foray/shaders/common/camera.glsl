/*
    common/camera.glsl

    Contains a definition and layout macros for the Camera Ubo

    C++: src/scenegraph/camerauboblock.hpp
*/

#ifdef BIND_CAMERA_UBO
#ifndef SET_CAMERA_UBO
#define SET_CAMERA_UBO 0
#endif

layout(set = SET_CAMERA_UBO, binding = BIND_CAMERA_UBO) uniform CameraUboBlock
{
    /// @brief Current frames projection matrix
    mat4 ProjectionMatrix;
    mat4 ViewMatrix;
    mat4 PreviousProjectionMatrix;
    mat4 PreviousViewMatrix;
    mat4 ProjectionViewMatrix;
    mat4 PreviousProjectionViewMatrix;
    mat4 InverseViewMatrix;
    mat4 InverseProjectionMatrix;
}
Camera;

#endif