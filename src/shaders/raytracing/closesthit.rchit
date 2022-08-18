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
#extension GL_KHR_vulkan_glsl : enable
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_nonuniform_qualifier : enable
// #extension GL_EXT_scalar_block_layout : enable

#include "bindpoints.glsl"

layout(location = 0) rayPayloadInEXT vec3 hitValue;
hitAttributeEXT vec3 attribs;

#include "../materialbuffer.glsl"

layout(binding = BIND_VERTICES,  set = 0) readonly buffer Vertices { float v[]; } vertices;
layout(binding = BIND_INDICES,   set = 0) readonly buffer Indices { uint i[]; } indices;

#include "geometrymeta.glsl"

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

	v.materialIndex = floatBitsToInt(vertices.v[m * index + 11]);

	return v;
}

void main()
{
	const vec3 barycentricCoords = vec3(1.0f - attribs.x - attribs.y, attribs.x, attribs.y);
	
	GeometryMeta geometa = GetGeometryMeta(uint(gl_InstanceCustomIndexEXT), uint(gl_GeometryIndexEXT));

	uint firstIndex = geometa.IndexBufferOffset + gl_PrimitiveID * 3;

	// get primitive indices
	uvec3 index = uvec3(indices.i[firstIndex + 0], indices.i[firstIndex + 1], indices.i[firstIndex + 2]);

	// get primitive vertices
	S_Vertex v0 = getVertex(index.x);
	S_Vertex v1 = getVertex(index.y);
	S_Vertex v2 = getVertex(index.z);

	// calculate uv
    vec2 uv = v0.uv * barycentricCoords.x + v1.uv * barycentricCoords.y + v2.uv * barycentricCoords.z;

	// get material 
	MaterialBufferObject material = GetMaterialOrFallback(v0.materialIndex);
	MaterialProbe probe = ProbeMaterial(material, uv);

	// hitValue = probe.BaseColor.xyz;
	// hitValue.xy = uv;
	// hitValue.y = v1.materialIndex;
	// hitValue.z = v1.materialIndex;
	//hitValue = probe.Normal;

	vec3 modelspacepos = v0.pos * barycentricCoords.x + v1.pos * barycentricCoords.y + v2.pos * barycentricCoords.z;
	vec3 worldspacepos = vec3(gl_ObjectToWorldEXT * vec4(modelspacepos, 1.f));

	// Interpolate normal of hitpoint
	vec3 modelspacenormal = v0.normal * barycentricCoords.x + v1.normal * barycentricCoords.y + v2.normal * barycentricCoords.z;
	mat3 normalCalcMatrix = transpose(mat3(mat4x3(gl_WorldToObjectEXT)));
	vec3 worldspacenormal = normalize(normalCalcMatrix * modelspacenormal);

	hitValue = worldspacenormal;

	//hitValue.xyz = vec3(mod(gl_PrimitiveID,100)/100.0f);
	//hitValue.xyz = vec3(indices.i[3 * gl_PrimitiveID]/10000.f);
	//hitValue = vec3(gl_PrimitiveID/10000.0f);
	//hitValue = normalize(v0.normal);
	

}
