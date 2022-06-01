#version 450

layout (location = 0) in vec3 inPos;			// Vertex position in model space
layout (location = 1) in vec3 inNormal;			// Vertex normal
layout (location = 2) in vec3 inTangent;		// Vertex tangent
layout (location = 3) in vec2 inUV;				// UV coordinates
layout (location = 4) in int inMaterialIndex;	// Material Index
layout (location = 5) in int inMeshId;			// Mesh Id

layout (location = 0) out vec3 outWorldPos;				// Vertex position in world space
layout (location = 1) out vec4 outDevicePos;			// Vertex position in normalized device space (current frame)
layout (location = 2) out vec4 outOldDevicePos;			// Vertex position in normalized device space (previous frame)
layout (location = 3) out vec3 outNormal; 				// Normal in world space
layout (location = 4) out vec3 outTangent;				// Tangent in world space
layout (location = 5) out vec2 outUV;					// UV coordinates
layout (location = 6) flat out int outMaterialIndex;	// Material Index
layout (location = 7) flat out int outMeshId;			// Mesh Id

struct Mx
{
	mat4 TransformMat;
};

layout(set = 0, binding = 2 ) buffer TransformationMatrices
{
	Mx matrices[];
};

void main() 
{
	mat4 ProjMat;
	mat4 ViewMat;
	mat4 ModelMat;
	mat4 ProjMatPrev;
	mat4 ViewMatPrev;
	mat4 ModelMatPrev;

	// Get transformations out of the way
	outWorldPos = inPos;
	//outDevicePos = ProjMat * ViewMat * vec4(inPos, 1.f);
	outDevicePos = vec4(inPos, 1.f) + vec4(0.0,0.1,0.0,0);
	gl_Position = outDevicePos;
	outOldDevicePos = ProjMatPrev * ViewMatPrev * vec4(inPos, 1.f);

	outUV = inUV;

	// Normal in world space
	mat3 mNormal = transpose(inverse(mat3(1.0)));
	outNormal = mNormal * normalize(inNormal);	
	outTangent = mNormal * normalize(inTangent);
	
	// Set vertex color passthrough
	outMaterialIndex = inMaterialIndex;

	outMeshId = inMeshId;
}
