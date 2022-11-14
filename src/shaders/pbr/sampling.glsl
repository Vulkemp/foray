#ifndef SAMPLING_GLSL
#define SAMPLING_GLSL

#include "../common/lcrng.glsl"
#include "../common/gltf.glsl"
#include "constants.glsl"

struct HitSample
{
    // Normal
	vec3 Normal;
    // Incoming light direction
	vec3 wIn;
    // Outgoing light direction (to observer)
	vec3 wOut;
    // Incoming + outgoing normalized
	vec3 wHalf;
};

vec3 sampleHemiSphere(inout uint seed)
{
	float r1 = lcgFloat(seed);
	float r2 = lcgFloat(seed);

	float cosTheta = sqrt(1 - r1);
	float sinTheta = sqrt(1 - cosTheta * cosTheta);
	float phi = TWOPI * r2;

	return normalize(vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta));
}

// https://jcgt.org/published/0007/04/01/paper.pdf
// Input Ve: view direction
// Input alpha_x, alpha_y: roughness parameters
// Input U1, U2: uniform random numbers
// Output Ne: normal sampled with PDF D_Ve(Ne) = G1(Ve) * max(0, dot(Ve, Ne)) * D(Ne) / Ve.z
vec3 sampleGGXVNDF(uint seed, vec3 wOut, float alpha)
{
    float U1 = lcgFloat(seed);
    float U2 = lcgFloat(seed);

    // Section 3.2: transforming the view direction to the hemisphere configuration
    vec3 Vh = normalize(vec3(alpha * wOut.x, alpha * wOut.y, wOut.z));
    // Section 4.1: orthonormal basis (with special case if cross product is zero)
    float lensq = Vh.x * Vh.x + Vh.y * Vh.y;
    vec3 T1 = lensq > 0 ? vec3(-Vh.y, Vh.x, 0) * inversesqrt(lensq) : vec3(1,0,0);
    vec3 T2 = cross(Vh, T1);
    // Section 4.2: parameterization of the projected area
    float r = sqrt(U1);
    float phi = 2.0 * PI * U2;
    float t1 = r * cos(phi);
    float t2 = r * sin(phi);
    float s = 0.5 * (1.0 + Vh.z);
    t2 = (1.0 - s)*sqrt(1.0 - t1*t1) + s*t2;
    // Section 4.3: reprojection onto hemisphere
    vec3 Nh = t1*T1 + t2*T2 + sqrt(max(0.0, 1.0 - t1*t1 - t2*t2))*Vh;
    // Section 3.4: transforming the normal back to the ellipsoid configuration
    vec3 Ne = normalize(vec3(alpha * Nh.x, alpha * Nh.y, max(0.0, Nh.z)));
    return Ne;
}

// https://schuttejoe.github.io/post/ggximportancesamplingpart2/
// https://hal.archives-ouvertes.fr/hal-01509746/document

vec3 sampleGgxVndf(inout uint seed, in HitSample hit, in MaterialProbe probe)
{
    float roughness = probe.MetallicRoughness.g * probe.MetallicRoughness.g;
    // -- Stretch the view vector so we are sampling as though
    // -- roughness==1
    vec3 v = normalize(vec3(hit.wOut.x * roughness, hit.wOut.y, hit.wOut.z * roughness));

    // -- Build an orthonormal basis with v, t1, and t2
    vec3 t1 = (abs(v.y) < 0.999) ? normalize(cross(v, vec3(0, 1, 0))) : vec3(1, 0, 0);
    vec3 t2 = cross(t1, v);

    float u1 = lcgFloat(seed);
    float u2 = lcgFloat(seed);

    // -- Choose a point on a disk with each half of the disk weighted
    // -- proportionally to its projection onto direction v
    float a = 1.0f / (1.0f + v.y);
    float r = sqrt(u1);
    float phi = (u2 < a) ? (u2 / a) * PI  : PI + (u2 - a) / (1.0f - a) * PI;
    float p1 = r * cos(phi);
    float p2 = r * sin(phi) * ((u2 < a) ? 1.0f : v.y);

    // -- Calculate the normal in this stretched tangent space
    vec3 n = p1 * t1 + p2 * t2 + sqrt(max(0.0f, 1.0f - p1 * p1 - p2 * p2)) * v;

    // -- unstretch and normalize the normal
    return normalize(vec3(roughness * n.x,
                            max(0.0f, n.y),
                            roughness * n.z));
}

// Legacy
vec3 sampleGGX(inout uint seed, in mat3 reflectionTBN, float alpha)
{
    const float PI = 3.1415926535897932384626433;

	float r1 = lcgFloat(seed);
	float r2 = lcgFloat(seed);

	float cosTheta = sqrt((1 - r1) / ((alpha * alpha - 1) * r1 + 1));
	float sinTheta = sqrt(1 - cosTheta * cosTheta);
	float phi = TWOPI * r2;

	vec3 direction = vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
	direction = normalize(reflectionTBN * direction);

	return direction;
}

#endif  // SAMPLING_GLSL