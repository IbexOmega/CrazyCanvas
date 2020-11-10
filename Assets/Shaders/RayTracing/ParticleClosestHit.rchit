#version 460
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_buffer_reference : enable
#extension GL_EXT_scalar_block_layout : enable

#include "RayTracingInclude.glsl"
#include "../Helpers.glsl"
#include "../Defines.glsl"
#include "../Noise.glsl"

struct SRayHitDescription
{
	vec3	Position;
	vec3	Normal;
	vec3	Albedo;
};

layout(buffer_reference, buffer_reference_align = 16) buffer VertexBuffer 
{
	vec4 v[];
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
	uint particleIndex = gl_InstanceCustomIndexEXT;
	uint emmiterIndex = b_ParticleIndirectIndices.Val[particleIndex].EmitterIndex;
	SParticle particle = b_ParticleInstances.Val[particleIndex];
	SEmitter emitter = b_EmitterInstances.Val[emmiterIndex];
	SAtlasData atlasData 	= b_Atlases.Val[emitter.AtlasIndex];

	vec3 barycentricCoords = vec3(1.0f - attribs.x - attribs.y, attribs.x, attribs.y);

	const uvec3 tri = indices.t[gl_PrimitiveID];

	vec4 v0 = vertices.v[tri.x];
	vec4 v1 = vertices.v[tri.y];
	vec4 v2 = vertices.v[tri.z];

	vec2 v0TexCoord = (v0.xy + 1.f) * 0.5f;
	vec2 v1TexCoord = (v1.xy + 1.f) * 0.5f;
	vec2 v2TexCoord = (v2.xy + 1.f) * 0.5f;
	
	uint tx = particle.TileIndex % atlasData.ColCount;
	uint ty = particle.TileIndex / atlasData.RowCount;
	vec2 factor = vec2(atlasData.TileFactorX, atlasData.TileFactorY);

	v0TexCoord = v0TexCoord * factor + factor*vec2(float(tx), float(ty));
	v1TexCoord = v1TexCoord * factor + factor*vec2(float(tx), float(ty));
	v2TexCoord = v2TexCoord * factor + factor*vec2(float(tx), float(ty));

	vec2 texCoord = (v0TexCoord * barycentricCoords.x + v1TexCoord * barycentricCoords.y + v2TexCoord * barycentricCoords.z);
	vec3 shadingNormal		= vec3(1.0, 0.0, 0.0);

	vec3 albedo = emitter.Color.rgb * texture(u_TextureAtlases[emitter.AtlasIndex], texCoord).rgb;

	SRayHitDescription hitDescription;
	hitDescription.Position			= gl_WorldRayOriginEXT + normalize(gl_WorldRayDirectionEXT) * gl_HitTEXT;
	hitDescription.Normal			= shadingNormal;
	hitDescription.Albedo			= albedo;
	
	return hitDescription;
}

void main() 
{
	SRayHitDescription hitDescription = CalculateHitData();
	
	s_PrimaryPayload.HitPosition		= hitDescription.Position;
	s_PrimaryPayload.Normal				= hitDescription.Normal;
	s_PrimaryPayload.Albedo				= hitDescription.Albedo;
	s_PrimaryPayload.AO					= 1.0;
	s_PrimaryPayload.Roughness			= 1.0;
	s_PrimaryPayload.Metallic			= 0.0;
	s_PrimaryPayload.Distance			= gl_HitTEXT;
}