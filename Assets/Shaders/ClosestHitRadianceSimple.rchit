#version 460
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_ray_tracing : enable

#include "Helpers.glsl"
#include "Defines.glsl"
#include "RayTracingIncludeSimple.glsl"

layout(location = 0) rayPayloadInEXT SRadiancePayload s_RadiancePayload;

hitAttributeEXT vec3 attribs;

void main() 
{
    SRayHitDescription hitDescription = CalculateHitData(attribs, gl_InstanceCustomIndexEXT, gl_PrimitiveID, gl_ObjectToWorldEXT);

	vec3 hitPos = gl_WorldRayOriginEXT + normalize(gl_WorldRayDirectionEXT) * gl_HitTEXT;

	SMaterialParameters materialParameters = b_MaterialParameters.val[hitDescription.MaterialIndex];

	vec3 sampledAlbedo 		=		texture(u_SceneAlbedoMaps[hitDescription.MaterialIndex],    hitDescription.TexCoord).rgb;
	float sampledMetallic 	= 		texture(u_SceneMetallicMaps[hitDescription.MaterialIndex],	hitDescription.TexCoord).r;
	float sampledRoughness 	= 		texture(u_SceneRoughnessMaps[hitDescription.MaterialIndex],	hitDescription.TexCoord).r;

    vec3 albedo       		= pow(  materialParameters.Albedo.rgb * sampledAlbedo, vec3(GAMMA));
    float metallic    		= 		materialParameters.Metallic * sampledMetallic;
	float roughness   		= max(	materialParameters.Roughness * sampledRoughness, EPSILON);

	s_RadiancePayload.HitPosition		= hitPos;
	s_RadiancePayload.ShadingNormal		= hitDescription.ShadingNormal;
	s_RadiancePayload.GeometricNormal	= hitDescription.GeometricNormal;
	s_RadiancePayload.Albedo			= albedo;
	s_RadiancePayload.Emissive			= albedo * materialParameters.EmissionStrength / 100.0f; //Todo: We need to approximate the area of the surface here 
	s_RadiancePayload.Metallic			= metallic;
	s_RadiancePayload.Roughness			= roughness;
	s_RadiancePayload.Distance			= gl_HitTEXT;
	s_RadiancePayload.HitMask			= (b_PrimaryInstances.val[gl_InstanceID].Mask_IndirectArgIndex >> 24);
}