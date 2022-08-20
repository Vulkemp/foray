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

#ifdef BIND_GEOMETRYMETA
#ifndef SET_GEOMETRYMETA
#define SET_GEOMETRYMETA 0
#endif  // SET_GEOMETRYMETA
layout(set = SET_GEOMETRYMETA, binding = BIND_GEOMETRYMETA, std430) buffer readonly GeometryMetaBuffer
{
    GeometryMeta Array[];
}
GeometryMetas;

GeometryMeta GetGeometryMeta(in uint geometryCustomIndex, in uint geometryIndex)
{
    return GeometryMetas.Array[geometryCustomIndex + geometryIndex];
}


#endif  // BIND_GEOMETRYMETA
