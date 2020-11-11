#version 460
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_buffer_reference : enable
#extension GL_EXT_scalar_block_layout : enable

#include "RayTracingInclude.glsl"
#include "../Helpers.glsl"
#include "../Defines.glsl"
#include "../Noise.glsl"
#include "../Paint.glsl"

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

struct SPaintDescription
{
	vec3 	Normal;
	vec3 	Albedo;
	float 	Roughness;
	float 	Interpolation;
};

struct SPaintSample
{
	float PaintAmount;
	uint Team;
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

SPaintSample SamplePaint(in ivec2 p, in uint paintMaskIndex)
{
	uvec2 paintData = floatBitsToUint(texelFetch(u_PaintMaskTextures[paintMaskIndex], p, 0).rg);

	uint clientTeam				= (paintData.g >> 1) & 0x7F;
	uint serverTeam				= (paintData.r >> 1) & 0x7F;
	uint clientPainting			= paintData.g & 0x1;

	SPaintSample paintSample;
	paintSample.PaintAmount		= float((paintData.r & 0x1) | (paintData.g & 0x1));
	paintSample.Team 			= clientPainting * clientTeam + (1 - clientPainting) * serverTeam;
	return paintSample;
}

SPaintDescription InterpolatePaint(in mat3 TBN, in vec3 position, in vec3 tangent, in vec3 bitangent, in vec2 texCoord, in uint paintMaskIndex)
{
	ivec2 paintMaskSize 		= textureSize(u_PaintMaskTextures[paintMaskIndex], 0);
	vec2 texelPos				= (texCoord * vec2(paintMaskSize));
	vec2 texelCenter;
	vec2 subTexel				= modf(texelPos, texelCenter);
	ivec2 iTexelCenter			= ivec2(texelCenter);

	SPaintSample paintSample00		= SamplePaint(iTexelCenter + ivec2(0, 0), 	paintMaskIndex);
	SPaintSample paintSample10		= SamplePaint(iTexelCenter + ivec2(1, 0), 	paintMaskIndex);
	SPaintSample paintSample01		= SamplePaint(iTexelCenter + ivec2(0, 1), 	paintMaskIndex);
	SPaintSample paintSample11		= SamplePaint(iTexelCenter + ivec2(1, 1), 	paintMaskIndex);

	vec3 paintColor00		= b_PaintMaskColor.val[paintSample00.Team].rgb;
	vec3 paintColor10 		= b_PaintMaskColor.val[paintSample10.Team].rgb;
	vec3 paintColor01 		= b_PaintMaskColor.val[paintSample01.Team].rgb;
	vec3 paintColor11 		= b_PaintMaskColor.val[paintSample11.Team].rgb;
	
	vec3 paintColorHor0 	= mix(paintColor00, paintColor10, subTexel.x);
	vec3 paintColorHor1 	= mix(paintColor01, paintColor11, subTexel.x);
	vec3 paintColorFinal	= mix(paintColorHor0, paintColorHor1, subTexel.y);

	float paintAmountHor0	= mix(paintSample00.PaintAmount, paintSample10.PaintAmount, subTexel.x);
	float paintAmountHor1	= mix(paintSample01.PaintAmount, paintSample11.PaintAmount, subTexel.x);
	float paintAmountFinal	= mix(paintAmountHor0, paintAmountHor1, subTexel.y);

	float h0 					= snoise(PAINT_NOISE_SCALE * (position));
	float h_u 					= snoise(PAINT_NOISE_SCALE * (position + PAINT_DELTA_NOISE * tangent));
	float h_v 					= snoise(PAINT_NOISE_SCALE * (position + PAINT_DELTA_NOISE * tangent));
	vec2 grad_h					= vec2(h0 - h_u, h0 - h_v);
	vec3 paintNormal			= normalize(vec3(PAINT_BUMPINESS * grad_h, sqrt(1.0f - (grad_h.x * grad_h.x) - (grad_h.y * grad_h.y))));
	vec3 noPaintNormal00		= normalize(vec3(-1.0f,  1.0f, 1.0f));
	vec3 noPaintNormal10		= normalize(vec3( 1.0f,  1.0f, 1.0f));
	vec3 noPaintNormal01		= normalize(vec3(-1.0f, -1.0f, 1.0f));
	vec3 noPaintNormal11		= normalize(vec3( 1.0f, -1.0f, 1.0f));
	vec3 combinedNormal			= TBN * normalize(
		paintNormal + 
		(1.0f - paintSample00.PaintAmount) * noPaintNormal00 +
		(1.0f - paintSample10.PaintAmount) * noPaintNormal10 +
		(1.0f - paintSample01.PaintAmount) * noPaintNormal01 +
		(1.0f - paintSample11.PaintAmount) * noPaintNormal11
	);

	SPaintDescription paintDescription;

	paintDescription.Normal			= combinedNormal;
	paintDescription.Albedo 		= paintColorFinal;
	paintDescription.Roughness 		= PAINT_ROUGHNESS;
	paintDescription.Interpolation	= paintAmountFinal;

	return paintDescription;
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