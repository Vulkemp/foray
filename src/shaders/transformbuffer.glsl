#ifdef BIND_TRANSFORMBUFFER
#ifndef SET_TRANSFORMBUFFER
#define SET_TRANSFORMBUFFER 0
#endif 

struct TransformState
{
    mat4 CurrentWorldMatrix;
    mat4 PreviousWorldMatrix;
};

layout (set = SET_TRANSFORMBUFFER, binding = BIND_TRANSFORMBUFFER) readonly buffer TransformBuffer_T { TransformState Array[]; } TransformBuffer;

TransformState GetTransform(in uint transformBufferIndex) {
    return TransformBuffer.Array[transformBufferIndex];
}

#endif 