// See https://64.github.io/tonemapping/

vec3 ReinhardTonemapper(vec3 v, float max_white_l)
{
    float l = dot(v, vec3(0.299, 0.587, 0.114));
    vec3 tv = v / (1.0f + v);
    return mix(v / (1.0f + l), tv, tv);
}