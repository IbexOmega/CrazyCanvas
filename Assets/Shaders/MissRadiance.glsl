#version 460
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_ray_tracing : enable

#include "Helpers.glsl"
#include "RayTracingInclude.glsl"

layout(location = 0) rayPayloadInEXT SRadiancePayload s_RadiancePayload;

void main() 
{
    s_RadiancePayload.ScatterPosition	= vec3(0.0f);
    s_RadiancePayload.Albedo			= vec3(0.0f);
    s_RadiancePayload.F_0				= vec3(0.0f);
    s_RadiancePayload.Alpha				= 0.0f;
    s_RadiancePayload.Distance			= 0.0f;
    s_RadiancePayload.LocalToWorld 		= mat3(1.0f);
}