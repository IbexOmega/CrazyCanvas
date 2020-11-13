#version 460
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_buffer_reference : enable
#extension GL_EXT_scalar_block_layout : enable

#include "RayTracingInclude.glsl"

struct SRayHitDescription
{
	vec3	Position;
	vec3	Normal;
	vec3	Tangent;
	vec3	Bitangent;
	mat3 	TBN;
	vec2	TexCoord;
	uint	MaterialIndex;
	uint	PaintMaskIndex;
};

layout(buffer_reference, buffer_reference_align = 16) buffer VertexBuffer 
{
	SVertex v[];
};

layout(buffer_reference, buffer_reference_align = 4, scalar) buffer IndexBuffer 
{
	uvec3 t[];
};

layout(shaderRecordEXT) buffer SBTData 
{
	VertexBuffer vertices;
	IndexBuffer indices;
};

layout(location = 0) rayPayloadInEXT SPrimaryPayload s_PrimaryPayload;

hitAttributeEXT vec3 attribs;

SRayHitDescription CalculateHitData()
{
	vec3 barycentricCoords = vec3(1.0f - attribs.x - attribs.y, attribs.x, attribs.y);

	const uvec3 tri = indices.t[gl_PrimitiveID];

	SVertex v0 = vertices.v[tri.x];
	SVertex v1 = vertices.v[tri.y];
	SVertex v2 = vertices.v[tri.z];

	vec3 T = normalize(v0.Tangent.xyz * barycentricCoords.x + v1.Tangent.xyz * barycentricCoords.y + v2.Tangent.xyz * barycentricCoords.z);
	vec3 N = normalize(v0.Normal.xyz * barycentricCoords.x + v1.Normal.xyz * barycentricCoords.y + v2.Normal.xyz * barycentricCoords.z);

	T = normalize(gl_ObjectToWorldEXT * vec4(T, 0.0f));
	N = normalize(gl_ObjectToWorldEXT * vec4(N, 0.0f));
	T = normalize(T - dot(T, N) * N);
	vec3 B = cross(N, T);
	mat3 TBN = mat3(T, B, N);

	vec2 texCoord = (v0.TexCoord.xy * barycentricCoords.x + v1.TexCoord.xy * barycentricCoords.y + v2.TexCoord.xy * barycentricCoords.z);

	uint materialIndex		= (gl_InstanceCustomIndexEXT & 0xFF00) >> 8;
	uint paintMaskIndex		= gl_InstanceCustomIndexEXT & 0xFF;

	vec3 shadingNormal		= texture(u_NormalMaps[materialIndex], texCoord).xyz;
	shadingNormal			= normalize(shadingNormal * 2.0f - 1.0f);
	shadingNormal			= TBN * shadingNormal;

	SRayHitDescription hitDescription;
	hitDescription.Position			= gl_WorldRayOriginEXT + normalize(gl_WorldRayDirectionEXT) * gl_HitTEXT;
	hitDescription.Normal			= shadingNormal;
	hitDescription.Tangent			= T;
	hitDescription.Bitangent		= B;
	hitDescription.TBN				= TBN;
	hitDescription.TexCoord			= texCoord;
	hitDescription.MaterialIndex	= materialIndex;
	hitDescription.PaintMaskIndex	= paintMaskIndex;
	
	return hitDescription;
}

void main() 
{
	SRayHitDescription hitDescription = CalculateHitData();
	SPaintDescription paintDescription = InterpolatePaint(hitDescription.TBN, hitDescription.Position, hitDescription.Tangent, hitDescription.Bitangent, hitDescription.TexCoord, hitDescription.PaintMaskIndex);
	
	SMaterialParameters materialParameters = u_MaterialParameters.val[hitDescription.MaterialIndex];

	vec3 sampledAlbedo 		= texture(u_AlbedoMaps[hitDescription.MaterialIndex],			hitDescription.TexCoord).rgb;
	vec3 sampledMaterial	= texture(u_CombinedMaterialMaps[hitDescription.MaterialIndex],	hitDescription.TexCoord).rgb;

	vec3 albedo				= pow(  materialParameters.Albedo.rgb * sampledAlbedo, vec3(GAMMA));
	float ao				= 		materialParameters.AO * sampledMaterial.r;
	float roughness			= 		materialParameters.Roughness * sampledMaterial.g;
	float metallic			= 		materialParameters.Metallic * sampledMaterial.b;

	s_PrimaryPayload.HitPosition		= hitDescription.Position;
	s_PrimaryPayload.Normal				= mix(hitDescription.Normal, paintDescription.Normal, paintDescription.Interpolation);
	s_PrimaryPayload.Albedo				= mix(albedo, paintDescription.Albedo, paintDescription.Interpolation);
	s_PrimaryPayload.AO					= ao;
	s_PrimaryPayload.Roughness			= mix(roughness, paintDescription.Roughness, paintDescription.Interpolation);
	s_PrimaryPayload.Metallic			= metallic;
	s_PrimaryPayload.Distance			= gl_HitTEXT;
}