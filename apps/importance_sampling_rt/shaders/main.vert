#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 albedo;
layout (location = 0) out vec3 fragColor;

void main() {
    gl_Position = vec4(pos, 1.0);
    fragColor = albedo;
}