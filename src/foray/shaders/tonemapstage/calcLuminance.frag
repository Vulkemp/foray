#version 460
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_GOOGLE_include_directive : enable

layout (location = 0) in vec2 UV;

layout (set = 0, binding = 0) uniform sampler2D Input;

layout (location = 0) out float Output;

void main()
{
    vec3 color = texture(Input, UV).rgb;
    Output = dot(color, vec3(0.299, 0.587, 0.114));
}