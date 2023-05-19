/*
    rt_common/geobuffers.glsl

	Layout macros for vertex and index buffers aswell as methods for accessing indices and vertices
*/

#ifndef GEOBUFFERS_GLSL
#define GEOBUFFERS_GLSL

layout(binding = BIND_VERTICES,  set = SET_VERTICES) readonly buffer Vertices { float v[]; } vertices;
layout(binding = BIND_INDICES,   set = SET_INDICES) readonly buffer Indices { uint i[]; } indices;
#include "geometrymeta.glsl"

#include "../common/vertex.glsl"

Vertex GetVertex(uint index)
{

	const int m = 11; // 11 floats per vertex

	Vertex v;
	v.Pos.x = vertices.v[m * index + 0];
	v.Pos.y = vertices.v[m * index + 1];
	v.Pos.z = vertices.v[m * index + 2];

	v.Normal.x = vertices.v[m * index + 3];
	v.Normal.y = vertices.v[m * index + 4];
	v.Normal.z = vertices.v[m * index + 5];

	v.Tangent.x = vertices.v[m * index + 6];
	v.Tangent.y = vertices.v[m * index + 7];
	v.Tangent.z = vertices.v[m * index + 8];

	v.Uv.x = vertices.v[m * index + 9];
	v.Uv.y = vertices.v[m * index + 10];

	return v;
}

void GetVertices(uvec3 indices, out Vertex v0, out Vertex v1, out Vertex v2)
{
    v0 = GetVertex(indices.x);
    v1 = GetVertex(indices.y);
    v2 = GetVertex(indices.z);
}

uvec3 GetIndices(in GeometryMeta geometa, in uint primitiveId)
{
	uint firstIndex = geometa.IndexBufferOffset + primitiveId * 3;
	return uvec3(indices.i[firstIndex + 0], indices.i[firstIndex + 1], indices.i[firstIndex + 2]);
}

#endif // GEOBUFFERS_GLSL