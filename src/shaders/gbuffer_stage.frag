#version 450
#extension GL_GOOGLE_include_directive : enable
#extension GL_KHR_vulkan_glsl: enable

/*
#define BIND_MATERIAL_BUFFER 0
#define BIND_TEXTURES_BUFFER 1
// #include "materialbuffer.glsl"

#include "gltf.glsl"

#ifdef BIND_MATERIAL_BUFFER
#ifndef SET_MATERIAL_BUFFER
#define SET_MATERIAL_BUFFER 0
#endif // SET_MATERIAL_BUFFER
layout(set = SET_MATERIAL_BUFFER, binding = BIND_MATERIAL_BUFFER ) readonly buffer MaterialBuffer { MaterialBufferObject Array[MATERIAL_BUFFER_COUNT]; } Materials;
#endif // BIND_MATERIAL_BUFFER

#ifdef BIND_TEXTURES_BUFFER
#ifndef SET_TEXTURES_BUFFER
#define SET_TEXTURES_BUFFER 0
#endif // SET_TEXTURES_BUFFER
layout(set = SET_TEXTURES_BUFFER, binding = BIND_TEXTURES_BUFFER ) uniform sampler2D Textures[TEXTURE_BUFFER_COUNT];
#endif // BIND_TEXTURES_BUFFER
*/

// TEMP
struct MaterialBufferObject  // 52 Bytes, aligned to 16 bytes causes size to be padded to a total of 64 bytes
{
    vec4  BaseColorFactor;                // Base Color / Albedo Factor
    float MetallicFactor;                 // Metallic Factor
    vec3  EmissiveFactor;                 // Emissive Factor
    float RoughnessFactor;                // Roughness Factor
    int   BaseColorTextureIndex;          // Texture Index for BaseColor
    int   MetallicRoughnessTextureIndex;  // Texture Index for MetallicRoughness
    int   EmissiveTextureIndex;           // Texture Index for Emissive
    int   NormalTextureIndex;             // Texture Index for Normal
};
layout(set = 0, binding = 0 ) readonly buffer MaterialBuffer { MaterialBufferObject Array[1]; } Materials;
layout(set = 0, binding = 1 ) uniform sampler2D Textures[];

layout (location = 0) in vec3 inWorldPos;			// Vertex position in world space
layout (location = 1) in vec4 inDevicePos;			// Vertex position in normalized device space (current frame)
layout (location = 2) in vec4 inOldDevicePos;		// Vertex position in normalized device space (previous frame)
layout (location = 3) in vec3 inNormal; 			// Normal in world space
layout (location = 4) in vec3 inTangent;			// Tangent in world space
layout (location = 5) in vec2 inUV;					// UV coordinates
layout (location = 6) flat in int inMaterialIndex;	// Material Index

layout (location = 0) out vec4 outPosition;			// Fragment position in world spcae
layout (location = 1) out vec4 outNormal;			// Fragment normal in world space
layout (location = 2) out vec4 outAlbedo;			// Fragment raw albedo
layout (location = 3) out vec2 outMotion;			// Fragment screenspace motion delta
//layout (location = 4) out int outMaterialIndex; 	// Material Index
layout (location = 4) out int outMeshId;			// Fragment mesh id


#define BIND_INSTANCE_PUSHC
#include "gltf_pushc.glsl"


void main() 
{
	// TEMP
	outPosition = vec4(1,1,1.0f, 1.0);
	outNormal = vec4(1,1,1.0f, 1.0);
	outAlbedo = vec4(1,1,1.0f, 1.0);
	outMotion = vec2(0.5, 1.0);
	outMeshId = PushConstant.MeshId;
/*
	outPosition = vec4(inWorldPos, 1.0);

	// Calculate normal in tangent space
	MaterialBufferObject material = GetMaterialOrFallback(inMaterialIndex, Materials.Array);

	MaterialProbe probe = ProbeMaterial(material, inUV, Textures);

	if (probe.Normal == vec3(0.f, 1.f, 0.f))
	{
		outNormal = vec4(normalize(inNormal), 0.0);
	}
	else
	{
		vec3 N = normalize(inNormal);
		vec3 T = normalize(inTangent);
		vec3 B = cross(N, T);
		mat3 TBN = mat3(T, B, N);
		vec3 tnorm = TBN * normalize(probe.Normal * 2.0 - vec3(1.0));
		outNormal = vec4(tnorm, 0.0);
	}

	// Get albedo. 
	outAlbedo =	probe.BaseColor;

	// Calculate motion delta
	vec2 screenPos = inDevicePos.xy / inDevicePos.w;
	vec2 old_screenPos = inOldDevicePos.xy / inOldDevicePos.w;
   	outMotion = (old_screenPos-screenPos) * 0.5;

	outMeshId = inMeshId;
	*/
}

