/*
	rt_basic/miss.rmiss

	Basic miss shader
*/

#version 460
#extension GL_GOOGLE_include_directive : enable // Include files
#extension GL_EXT_ray_tracing : enable // Raytracing

// Declare hitpayloads

#define HITPAYLOAD_IN
#include "../rt_common/payload.glsl"

// TODO: Add environmentmap sampling

void main()
{
    ReturnPayload.Radiance = vec3(0.f, 0.f, 0.4f);
}