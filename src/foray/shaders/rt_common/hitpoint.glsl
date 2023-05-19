struct HitPoint
{
	vec3 BarycentricCoords;
	Vertex Vertices[3];
	MaterialProbe Probe;
	vec3 WorldSpacePosition;
	vec3 WorldSpaceNormal;
	mat3 TBN;
};

HitPoint GetHitpoint(in vec2 hitAttribs)
{
	HitPoint r;
	// Calculate barycentric coords from hitAttribute values
	r.BarycentricCoords = vec3(1.0f - attribs.x - attribs.y, attribs.x, attribs.y);
	
	// Get geometry meta info
	GeometryMeta geometa = GetGeometryMeta(uint(gl_InstanceCustomIndexEXT), uint(gl_GeometryIndexEXT));

	// get primitive indices
	const uvec3 indices = GetIndices(geometa, uint(gl_PrimitiveID));

	// get primitive vertices
	Vertex v0, v1, v2;
	GetVertices(indices, v0, v1, v2);
	r.Vertices[0] = v0;
	r.Vertices[1] = v1;
	r.Vertices[2] = v2;

	// calculate uv
    const vec2 uv = v0.Uv * r.BarycentricCoords.x + v1.Uv * r.BarycentricCoords.y + v2.Uv * r.BarycentricCoords.z;

	// get material 
	MaterialBufferObject material = GetMaterialOrFallback(geometa.MaterialIndex);
	r.Probe = ProbeMaterial(material, uv);

	// Calculate model and worldspace positions
	const vec3 posModelSpace = v0.Pos * r.BarycentricCoords.x + v1.Pos * r.BarycentricCoords.y + v2.Pos * r.BarycentricCoords.z;
	r.WorldSpacePosition = vec3(gl_ObjectToWorldEXT * vec4(posModelSpace, 1.f));

	// Interpolate TBN of hitpoint (encapsulates normal, tangent and bitangent)
	const vec3 normalModelSpace = v0.Normal * r.BarycentricCoords.x + v1.Normal * r.BarycentricCoords.y + v2.Normal * r.BarycentricCoords.z;
	const vec3 tangentModelSpace = v0.Tangent * r.BarycentricCoords.x + v1.Tangent * r.BarycentricCoords.y + v2.Tangent * r.BarycentricCoords.z;
	const mat3 modelMatTransposedInverse = transpose(mat3(mat4x3(gl_WorldToObjectEXT)));
	vec3 normalWorldSpace = normalize(modelMatTransposedInverse * normalModelSpace);
	const vec3 tangentWorldSpace = normalize(tangentModelSpace);
	
	r.TBN = CalculateTBN(normalWorldSpace, tangentWorldSpace);
	r.WorldSpaceNormal = ApplyNormalMap(r.TBN, r.Probe);
	return r;
}

