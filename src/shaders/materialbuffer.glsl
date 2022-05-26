#include "gltf.glsl"

#ifdef BIND_MATERIAL_BUFFER
#ifndef SET_MATERIAL_BUFFER
#define SET_MATERIAL_BUFFER 0
#endif // SET_MATERIAL_BUFFER
layout(set = SET_MATERIAL_BUFFER, binding = BIND_MATERIAL_BUFFER ) readonly buffer MaterialBuffer { MaterialBufferObject Array[MATERIAL_BUFFER_COUNT]; } Materials;
#endif // BIND_MATERIAL_BUFFER

#ifdef BIND_TEXTURES_BUFFER
#ifndef SET_TEXTURES_BUFFER
#define SET_TEXTURES_BUFFER 0
#endif // SET_TEXTURES_BUFFER
layout(set = SET_TEXTURES_BUFFER, binding = BIND_TEXTURES_BUFFER ) uniform sampler2D Textures[TEXTURE_BUFFER_COUNT];
#endif // BIND_TEXTURES_BUFFER
