#version 460
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_GOOGLE_include_directive : enable

#include "../postprocessing/tonemap.glsl"

layout (location = 0) in vec2 UV;

layout (set = 0, binding = 0) uniform sampler2D Input;

layout (location = 0) out vec4 Output;

layout (push_constant) uniform PushC_T
{
    uint TonemapperIdx;
    float Exposure;
};

void main()
{
    vec2 FlipUV = vec2(UV.x, 1.f - UV.y);
    vec3 colorIn = texture(Input, FlipUV).rgb;
    Output = vec4(ApplyTonemap(TonemapperIdx, colorIn, Exposure), 1);
}