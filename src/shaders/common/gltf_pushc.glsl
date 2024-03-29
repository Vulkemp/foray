/*
    common/gltf_pushc.glsl

    Definition for the push constant used for rasterized rendering of scenes

    C++: src/scenegraph/foray_scenedrawing.hpp
*/

#ifdef BIND_PUSHC

layout(push_constant) uniform PushConstantBlock
{
    uint TransformBufferOffset;
    int  MaterialIndex;
}
PushConstant;
#endif