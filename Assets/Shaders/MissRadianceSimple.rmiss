#version 460
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_ray_tracing : enable

#include "Helpers.glsl"
#include "RayTracingIncludeSimple.glsl"

layout(location = 0) rayPayloadInEXT SRadiancePayload s_RadiancePayload;

void main() 
{
    s_RadiancePayload.HitPosition	    = vec3(0.0f);
    s_RadiancePayload.ShadingNormal     = vec3(0.0f);
    s_RadiancePayload.GeometricNormal   = vec3(0.0f);
    s_RadiancePayload.Albedo			= vec3(0.0f);
    s_RadiancePayload.Emissive			= vec3(0.0f);
    s_RadiancePayload.Metallic			= 0.0f;
    s_RadiancePayload.Roughness			= 0.0f;
    s_RadiancePayload.Distance			= 0.0f;
}