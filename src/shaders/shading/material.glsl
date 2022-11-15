#ifndef MATERIAL_GLSL
#define MATERIAL_GLSL

// See https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#metal-brdf-and-dielectric-brdf

float heaviside(float v)
{
	return v > 0 ? 1.0 : 0.0;
}

float specularBrdf(in HitSample hit, float alpha)
{
	float alpha2 = alpha * alpha;
	float nDotH = dot(hit.Normal, hit.wHalf);
	float nDotL = dot(hit.wIn, hit.Normal);
	float nDotV = dot(hit.wOut, hit.Normal);
    // GGX microphacet distribution
	float D = (alpha2 * heaviside(nDotH)) / (PI * pow((nDotH * nDotH) * (alpha2 - 1) + 1, 2));
    // Visibility function
	float V0 = heaviside(dot(hit.wIn, hit.wHalf)) / (abs(nDotL) + sqrt(alpha2 + (1 - alpha2) * nDotL * nDotL));
	float V1 = heaviside(dot(hit.wHalf, hit.wOut)) / (abs(nDotV) + sqrt(alpha2 + (1 - alpha2) * nDotV * nDotV));
	return V0 * V1 * D;
}

vec3 diffuseBrdf(vec3 rgb)
{
	return (1 / PI) * rgb;
}

vec3 EvaluateMaterial(in HitSample hit, in MaterialBufferObject material, in MaterialProbe probe)
{
	vec3 halfVec = normalize(hit.wIn + hit.wOut);

	float tempVDotH = pow(1 - abs(dot(hit.wOut, hit.wHalf)), 5);

	vec4 baseColor = probe.BaseColor;
	float alpha = probe.MetallicRoughness.g;
	// float alpha = probe.MetallicRoughness.g * probe.MetallicRoughness.g;

    float specular = clamp(specularBrdf(hit, alpha), 0, 1);

	vec3 metalBrdf = specular * (baseColor.rgb + (1 - baseColor.rgb) * tempVDotH);

    float f0 = pow((1.0 - material.IndexOfRefraction) / (1.0 + material.IndexOfRefraction), 2);

	vec3 dielectricBrdf = mix(diffuseBrdf(baseColor.rgb), vec3(specular), f0 + (1 - f0) * tempVDotH);
	return mix(dielectricBrdf, metalBrdf, probe.MetallicRoughness.r);
}

#endif // MATERIAL_GLSL