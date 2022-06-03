#ifdef BIND_CAMERA_UBO
#ifndef SET_CAMERA_UBO
#define SET_CAMERA_UBO 0
#endif

layout (set = SET_CAMERA_UBO, binding = BIND_CAMERA_UBO ) uniform CameraUboBlock 
{
    mat4 ProjectionMatrix;
    mat4 ViewMatrix;
    mat4 PreviousProjectionMatrix;
    mat4 PreviousViewMatrix;
} Camera;

#endif