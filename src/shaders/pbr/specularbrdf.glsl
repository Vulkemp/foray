#ifndef SPECULAR_BRDF_GLSL
#define SPECULAR_BRDF_GLSL

#include "../common/random.glsl"

// Todo
vec3 sampleGGX(inout uint seed, in mat3 reflectionTBN, float alpha)
{
    const float PI = 3.1415926535897932384626433;

	float r1 = RandomFp32(seed);
	float r2 = RandomFp32(seed);

	float cosTheta = sqrt((1 - r1) / ((alpha * alpha - 1) * r1 + 1));
	float sinTheta = sqrt(1 - cosTheta * cosTheta);
	float phi = 2 * PI * r2;

	vec3 direction = vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
	direction = reflectionTBN * direction;

	return direction;
}

#endif  // SPECULAR_BRDF_GLSL