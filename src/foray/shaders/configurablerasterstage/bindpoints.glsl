/*
    gbuffer/bindpoints.glsl

    GBuffer bindpoints
*/

// Material Buffer
#define SET_MATERIAL_BUFFER 0
#define BIND_MATERIAL_BUFFER 0

// Texture Array
#define SET_TEXTURES_ARRAY 0
#define BIND_TEXTURES_ARRAY 1

// Camera Ubo
#define SET_CAMERA_UBO 0
#define BIND_CAMERA_UBO 2

// Current frames transforms
#define SET_TRANSFORMBUFFER_CURRENT 0
#define BIND_TRANSFORMBUFFER_CURRENT 3

// Previous frames transforms
#define SET_TRANSFORMBUFFER_PREVIOUS 0
#define BIND_TRANSFORMBUFFER_PREVIOUS 4

// Push Constants
#define BIND_PUSHC
