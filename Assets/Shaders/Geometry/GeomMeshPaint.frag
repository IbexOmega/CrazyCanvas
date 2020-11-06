#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shader_draw_parameters : enable
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_nonuniform_qualifier : enable

#include "../Defines.glsl"
#include "../Helpers.glsl"
#include "../Noise.glsl"
#include "../Paint.glsl"

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

layout(location = 0) in flat uint	in_MaterialSlot;
layout(location = 1) in vec3		in_WorldPosition;
layout(location = 2) in vec3		in_Normal;
layout(location = 3) in vec3		in_Tangent;
layout(location = 4) in vec3		in_Bitangent;
layout(location = 5) in vec2		in_TexCoord;
layout(location = 6) in vec4		in_ClipPosition;
layout(location = 7) in vec4		in_PrevClipPosition;
layout(location = 8) in flat uint	in_ExtensionIndex;

layout(binding = 1, set = BUFFER_SET_INDEX) readonly buffer MaterialParameters	{ SMaterialParameters val[]; }	b_MaterialParameters;
layout(binding = 2, set = BUFFER_SET_INDEX) readonly buffer PaintMaskColors		{ vec4 val[]; }	b_PaintMaskColor;

layout(binding = 0, set = TEXTURE_SET_INDEX) uniform sampler2D u_AlbedoMaps[];
layout(binding = 1, set = TEXTURE_SET_INDEX) uniform sampler2D u_NormalMaps[];
layout(binding = 2, set = TEXTURE_SET_INDEX) uniform sampler2D u_CombinedMaterialMaps[];

layout(binding = 0, set = DRAW_EXTENSION_SET_INDEX) uniform sampler2D u_PaintMaskTextures[];

layout(location = 0) out vec3 out_Albedo;
layout(location = 1) out vec4 out_AO_Rough_Metal_Valid;
layout(location = 2) out vec3 out_Compact_Normal;
layout(location = 3) out vec2 out_Velocity;

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
	vec3 normal		= normalize(in_Normal);
	vec3 tangent	= normalize(in_Tangent);
	vec3 bitangent	= normalize(in_Bitangent);
	vec2 texCoord	= in_TexCoord;

	mat3 TBN = mat3(tangent, bitangent, normal);
	
	vec3 sampledAlbedo				= texture(u_AlbedoMaps[in_MaterialSlot],			texCoord).rgb;
	vec3 sampledNormal				= texture(u_NormalMaps[in_MaterialSlot],			texCoord).rgb;
	vec3 sampledCombinedMaterial	= texture(u_CombinedMaterialMaps[in_MaterialSlot],	texCoord).rgb;

	SMaterialParameters materialParameters = b_MaterialParameters.val[in_MaterialSlot];
	SPaintDescription paintDescription = InterpolatePaint(TBN, in_WorldPosition, tangent, bitangent, in_TexCoord, in_ExtensionIndex);

	//0
	vec3 storedAlbedo			= pow(materialParameters.Albedo.rgb * sampledAlbedo, vec3(GAMMA));
	out_Albedo					= mix(storedAlbedo, paintDescription.Albedo, paintDescription.Interpolation);

	//1
	vec3 storedMaterial			= vec3(
									materialParameters.AO * sampledCombinedMaterial.b, 
									mix(materialParameters.Roughness * sampledCombinedMaterial.r, paintDescription.Roughness, paintDescription.Interpolation), 
									materialParameters.Metallic * sampledCombinedMaterial.g);
	out_AO_Rough_Metal_Valid	= vec4(storedMaterial, 1.0f);

	//2
	vec3 shadingNormal			= normalize((sampledNormal * 2.0f) - 1.0f);
	shadingNormal				= normalize(TBN * normalize(shadingNormal));
	out_Compact_Normal			= PackNormal(mix(shadingNormal, paintDescription.Normal, paintDescription.Interpolation));

	//3
	vec2 currentNDC				= (in_ClipPosition.xy / in_ClipPosition.w) * 0.5f + 0.5f;
	vec2 prevNDC				= (in_PrevClipPosition.xy / in_PrevClipPosition.w) * 0.5f + 0.5f;
	vec2 screenVelocity			= (prevNDC - currentNDC);
	out_Velocity				= vec2(screenVelocity);
}
