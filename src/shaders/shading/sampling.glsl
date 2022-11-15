#ifndef SAMPLING_GLSL
#define SAMPLING_GLSL

#include "../common/lcrng.glsl"
#include "../common/gltf.glsl"
#include "constants.glsl"

struct HitSample
{
    // Normal (hit -> normal)
	vec3 Normal;
    // Incoming light direction (hit -> light)
	vec3 wIn;
    // Outgoing light direction (hit -> observer)
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

// https://github.com/SaschaWillems/Vulkan-glTF-PBR/blob/master/data/shaders/genbrdflut.frag

// Based omn http://byteblacksmith.com/improvements-to-the-canonical-one-liner-glsl-rand-for-opengl-es-2-0/
float random(vec2 co)
{
	float a = 12.9898;
	float b = 78.233;
	float c = 43758.5453;
	float dt= dot(co.xy ,vec2(a,b));
	float sn= mod(dt,3.14);
	return fract(sin(sn) * c);
}

vec3 importanceSample_GGX(inout uint seed, float roughness, vec3 normal) 
{
	float r1 = lcgFloat(seed);
	float r2 = lcgFloat(seed);

	// Maps a 2D point to a hemisphere with spread based on roughness
	float alpha = roughness * roughness;
	float phi = 2.0 * PI * r1 + random(normal.xz) * 0.1;
	float cosTheta = sqrt((1.0 - r2) / (1.0 + (alpha*alpha - 1.0) * r2));
	float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
	vec3 H = vec3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);

	// Tangent space
	vec3 up = abs(normal.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
	vec3 tangentX = normalize(cross(up, normal));
	vec3 tangentY = normalize(cross(normal, tangentX));

	// Convert to world Space
	return normalize(tangentX * H.x + tangentY * H.y + normal * H.z);
}

#endif  // SAMPLING_GLSL