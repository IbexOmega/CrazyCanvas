#version 460
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_ray_tracing : enable
//#extension GL_EXT_debug_printf : enable

#include "Helpers.glsl"

struct SRayPayload
{
	vec3 OutgoingRadiance;
};

layout(location = 0) rayPayloadInEXT SRayPayload s_RayPayload;

void main() 
{
    s_RayPayload.OutgoingRadiance = vec3(0.0f, 0.0f, 0.0f);
}