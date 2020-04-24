#version 460
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_ray_tracing : enable

#include "helpers.glsl"

struct SRayPayload
{
	float Depth;
};

layout(location = 0) rayPayloadInEXT SRayPayload s_RayPayload;

void main() 
{
    s_RayPayload.Depth = 1.0f;
}