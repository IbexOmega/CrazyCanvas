#version 460
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_ray_tracing : enable

#include "Helpers.glsl"
#include "Defines.glsl"
#include "RayTracingInclude.glsl"

layout(location = 0) rayPayloadInEXT SRadiancePayload s_RadiancePayload;

hitAttributeEXT vec3 attribs;

void main() 
{
    SRayHitDescription hitDescription = CalculateHitData(attribs, gl_InstanceCustomIndexEXT, gl_PrimitiveID, gl_ObjectToWorldEXT);

	vec3 hitPos = gl_WorldRayOriginEXT + normalize(gl_WorldRayDirectionEXT) * gl_HitTEXT;

	//Define local Coordinate System
	vec3 tangent 	= vec3(0.0f);
	vec3 bitangent 	= vec3(0.0f);
	CreateCoordinateSystem(hitDescription.Normal, tangent, bitangent);

	//Create Transformation Matrices
	mat3 localToWorld = mat3(tangent, bitangent, hitDescription.Normal);

	SMaterialParameters materialParameters = b_MaterialParameters.val[hitDescription.MaterialIndex];

	vec3 sampledAlbedo 		= pow(  texture(u_SceneAlbedoMaps[hitDescription.MaterialIndex],    hitDescription.TexCoord).rgb, vec3(GAMMA));
	float sampledMetallic 	= 		texture(u_SceneMetallicMaps[hitDescription.MaterialIndex],	hitDescription.TexCoord).r;
	float sampledRoughness 	= 		texture(u_SceneRoughnessMaps[hitDescription.MaterialIndex],	hitDescription.TexCoord).r;

    vec3 albedo       		= materialParameters.Albedo.rgb * sampledAlbedo;
    float metallic    		= materialParameters.Metallic * sampledMetallic;
	float roughness   		= max(materialParameters.Roughness * sampledRoughness, EPSILON);

	s_RadiancePayload.ScatterPosition	= hitPos + hitDescription.Normal * RAY_NORMAL_OFFSET;
	s_RadiancePayload.Albedo			= albedo;
	s_RadiancePayload.Metallic			= metallic;
	s_RadiancePayload.Roughness			= roughness;
	s_RadiancePayload.Distance			= gl_HitTEXT;
	s_RadiancePayload.LocalToWorld 		= localToWorld;
}