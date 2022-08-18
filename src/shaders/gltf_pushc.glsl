#ifdef BIND_PUSHC
layout(push_constant) uniform PushConstantBlock
{
    uint TransformBufferOffset;
    int MaterialIndex;
}
PushConstant;
#endif