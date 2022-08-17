/* Copyright (c) 2019-2020, Sascha Willems
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 the "License";
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_nonuniform_qualifier : enable

#include "bindpoints.glsl"

layout(location = 0) rayPayloadInEXT vec3 hitValue;
hitAttributeEXT vec3 attribs;

layout(binding = BIND_VERTICES,  set = 0) readonly buffer Vertices { float v[]; } vertices;
layout(binding = BIND_INDICES,   set = 0) readonly buffer Indices { uint i[]; } indices;

#include "../materialbuffer.glsl"

struct S_Vertex
{
	vec3 pos;
	vec3 normal;
	vec3 tangent;
	vec2 uv;
	int  materialIndex;
};

S_Vertex getVertex(uint index)
{

	const int m = 12; // 12 floats per vertex

	S_Vertex v;
	v.pos.x = vertices.v[m * index + 0];
	v.pos.y = vertices.v[m * index + 1];
	v.pos.z = vertices.v[m * index + 2];

	v.normal.x = vertices.v[m * index + 3];
	v.normal.y = vertices.v[m * index + 4];
	v.normal.z = vertices.v[m * index + 5];

	v.tangent.x = vertices.v[m * index + 6];
	v.tangent.y = vertices.v[m * index + 7];
	v.tangent.z = vertices.v[m * index + 8];

	v.uv.x = vertices.v[m * index + 9];
	v.uv.y = vertices.v[m * index + 10];

	v.materialIndex = int(vertices.v[m * index + 11]);

	// old
	// S_Vertex v;
	// v.pos = d0.xyz;
	// v.normal = vec3(d0.w, d1.xy);
	// v.tangent = vec3(d1.zw, d2.x);
	// v.uv = d2.yz;
	// v.materialIndex = int(d2.w);

	return v;
}

void main()
{
	const vec3 barycentricCoords = vec3(1.0f - attribs.x - attribs.y, attribs.x, attribs.y);
	
	// get primitive indices
	ivec3 index = ivec3(indices.i[3 * gl_PrimitiveID], indices.i[3 * gl_PrimitiveID + 1], indices.i[3 * gl_PrimitiveID + 2]);

	// get primitive vertices
	S_Vertex v0 = getVertex(index.x);
	S_Vertex v1 = getVertex(index.y);
	S_Vertex v2 = getVertex(index.z);

	// calculate uv
    vec2 uv = v0.uv * barycentricCoords.x + v1.uv * barycentricCoords.y + v2.uv * barycentricCoords.z;

	// get material 
	MaterialBufferObject material = GetMaterialOrFallback(v0.materialIndex);
	MaterialProbe probe = ProbeMaterial(material, uv);

	hitValue = barycentricCoords;
	//hitValue.x = mod(indices.i[3 * gl_PrimitiveID], 255) / 255.f;
	hitValue = probe.BaseColor.xyz;
	hitValue.xy = uv;
	hitValue.y = v1.materialIndex;
	hitValue.z = v1.materialIndex;
	//hitValue = probe.Normal;

	hitValue = v0.pos * barycentricCoords.x + v1.pos * barycentricCoords.y + v2.pos * barycentricCoords.z;
	hitValue = vec3(gl_ObjectToWorldEXT * vec4(hitValue, 1.0));
	//hitValue = v0.normal * barycentricCoords.x + v1.normal * barycentricCoords.y + v2.normal * barycentricCoords.z;

	// Interpolate normal of hitpoint
	//vec3 normal = v0.normal * barycentricCoords.x + v1.normal * barycentricCoords.y + v2.normal * barycentricCoords.z;
	//hitValue = normalize(vec3(normal * gl_WorldToObjectEXT));

	//hitValue.xyz = vec3(mod(gl_PrimitiveID,100)/100.0f);
	//hitValue.xyz = vec3(indices.i[3 * gl_PrimitiveID]/10000.f);
	//hitValue = vec3(gl_PrimitiveID/10000.0f);
	//hitValue = normalize(v0.normal);
	

}
