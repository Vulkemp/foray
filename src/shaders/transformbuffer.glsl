#ifdef BIND_TRANSFORMBUFFER_CURRENT
#ifndef SET_TRANSFORMBUFFER_CURRENT
#define SET_TRANSFORMBUFFER_CURRENT 0
#endif 

layout (set = SET_TRANSFORMBUFFER_CURRENT, binding = BIND_TRANSFORMBUFFER_CURRENT) readonly buffer TransformBufferCurrent_T { mat4 Array[]; } TransformBufferCurrent;

mat4 GetCurrentTransform(in uint transformBufferIndex) {
    return TransformBufferCurrent.Array[transformBufferIndex];
}

#endif 
#ifdef BIND_TRANSFORMBUFFER_PREVIOUS
#ifndef SET_TRANSFORMBUFFER_PREVIOUS
#define SET_TRANSFORMBUFFER_PREVIOUS 0
#endif 

layout (set = SET_TRANSFORMBUFFER_PREVIOUS, binding = BIND_TRANSFORMBUFFER_PREVIOUS) readonly buffer TransformBufferPrevious_T { mat4 Array[]; } TransformBufferPrevious;

mat4 GetPreviousTransform(in uint transformBufferIndex) {
    return TransformBufferPrevious.Array[transformBufferIndex];
}

#endif 