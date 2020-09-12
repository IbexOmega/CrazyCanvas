#version 460
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_ray_tracing : enable

#include "Defines.glsl"
#include "Helpers.glsl"
#include "RayTracingIncludeSimple.glsl"

layout(location = 0) rayPayloadInEXT SRadiancePayload s_RadiancePayload;

void main() 
{
	vec3 sampledAlbedo 		= texture(u_Skybox, normalize(gl_WorldRayDirectionEXT)).rgb;

    vec3 albedo       		= pow(sampledAlbedo, vec3(GAMMA));

    s_RadiancePayload.HitPosition	    = vec3(0.0f);
    s_RadiancePayload.ShadingNormal     = vec3(0.0f);
    s_RadiancePayload.GeometricNormal   = vec3(0.0f);
	s_RadiancePayload.Albedo			= albedo;
	s_RadiancePayload.Emission			= vec3(0.0f);
	s_RadiancePayload.Metallic			= 0.0f;
	s_RadiancePayload.Roughness			= 0.0f;
	s_RadiancePayload.Distance			= 0.0f;
	s_RadiancePayload.HitMask			= 0x0;
}