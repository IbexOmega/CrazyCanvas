#version 460
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_ray_tracing : enable

#include "../Helpers.glsl"
#include "RayTracingInclude.glsl"

layout(location = 0) rayPayloadInEXT SPrimaryPayload s_PrimaryPayload;

void main() 
{
    s_PrimaryPayload.HitPosition		= vec3(0.0f);
	s_PrimaryPayload.Normal		        = vec3(0.0f);
	s_PrimaryPayload.Albedo			    = vec3(0.0f);
    s_PrimaryPayload.Roughness			= 0.0f;
	s_PrimaryPayload.Roughness			= 0.0f;
	s_PrimaryPayload.Metallic			= 0.0f;
	s_PrimaryPayload.Distance			= 0.0f;
}