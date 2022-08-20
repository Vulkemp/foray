/*
    rt_common/geometrymeta.glsl

    Defines the GeometryMeta struct which provides information applicable per Geometry per Blas

    C++: src/raytracing/hsk_geometrymetabuffer.hpp
*/

#ifndef GEOMETRYMETA_GLSL
#define GEOMETRYMETA_GLSL

/// @brief Struct for per Blas meta data
struct GeometryMeta
{
    /// @brief Index into the material information array
    int  MaterialIndex;
    /// @brief Offset into the Indexbuffer
    uint IndexBufferOffset;
};

#endif  //GEOMETRYMETA_GLSL
