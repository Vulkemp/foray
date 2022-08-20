/*
    common/vertex.glsl

    Contains the vertex struct definition (Vertex buffers don't follow alingment rules!)
*/

#ifndef VERTEX_GLSL
#define VERTEX_GLSL

/// @brief Vertex struct
struct Vertex
{
    /// @brief Position in model space
    vec3 Pos;
    /// @brief Normal in model space
    vec3 Normal;
    /// @brief Tangent in model space
    vec3 Tangent;
    /// @brief Uv coordinates for texture sampling
    vec2 Uv;
};

#endif // VERTEX_GLSL