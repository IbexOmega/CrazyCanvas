#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shader_draw_parameters : enable
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_nonuniform_qualifier : enable

#include "../Defines.glsl"
#include "../Helpers.glsl"

layout(binding = 2, set = BUFFER_SET_INDEX) readonly buffer PaintMaskColors		{ vec4 val[]; }					b_PaintMaskColor;
#include "../MeshPaintHelper.glsl"
#include "../MeshPaintFunc.glsl"

layout(location = 0) in flat uint	in_MaterialSlot;
layout(location = 1) in vec3		in_WorldPosition;
layout(location = 2) in vec3		in_Normal;
layout(location = 3) in vec3		in_Tangent;
layout(location = 4) in vec3		in_Bitangent;
layout(location = 5) in vec2		in_TexCoord;
layout(location = 6) in vec4		in_ClipPosition;
layout(location = 7) in vec4		in_PrevClipPosition;
layout(location = 8) in vec4		in_PaintInfo4;
layout(location = 9) in float 		in_PaintDist;

layout(binding = 1, set = BUFFER_SET_INDEX) readonly buffer MaterialParameters	{ SMaterialParameters val[]; }	b_MaterialParameters;

layout(binding = 0, set = TEXTURE_SET_INDEX) uniform sampler2D u_AlbedoMaps[];
layout(binding = 1, set = TEXTURE_SET_INDEX) uniform sampler2D u_NormalMaps[];
layout(binding = 2, set = TEXTURE_SET_INDEX) uniform sampler2D u_CombinedMaterialMaps[];


layout(location = 0) out vec3 out_Albedo;
layout(location = 1) out vec4 out_AO_Rough_Metal_Valid;
layout(location = 2) out vec3 out_Compact_Normal;
layout(location = 3) out vec2 out_Velocity;

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
	uint packedPaintInfo = 0;
	float dist = 1.f;
	GetVec4ToPackedPaintInfoAndDistance(in_WorldPosition, in_PaintInfo4, in_PaintDist, packedPaintInfo, dist);
	SPaintDescription paintDescription = InterpolatePaint(TBN, in_WorldPosition, tangent, bitangent, packedPaintInfo, dist);

	//0
	vec3 storedAlbedo			= pow(materialParameters.Albedo.rgb * sampledAlbedo, vec3(GAMMA));
	out_Albedo					= mix(storedAlbedo, paintDescription.Albedo, paintDescription.Interpolation);

	//1
	vec3 storedMaterial			= vec3(
									materialParameters.AO * sampledCombinedMaterial.r, 
									mix(materialParameters.Roughness * sampledCombinedMaterial.g, paintDescription.Roughness, paintDescription.Interpolation), 
									materialParameters.Metallic * sampledCombinedMaterial.b * float(paintDescription.Interpolation == 0.0f));
	out_AO_Rough_Metal_Valid	= vec4(storedMaterial, 1.0f);

	//2
	vec3 shadingNormal			= normalize((sampledNormal * 2.0f) - 1.0f);
	shadingNormal				= normalize(TBN * normalize(shadingNormal));
	out_Compact_Normal			= PackNormal(mix(shadingNormal, normalize(paintDescription.Normal+shadingNormal*0.2f), paintDescription.Interpolation));

	//3
	vec2 currentNDC				= (in_ClipPosition.xy / in_ClipPosition.w) * 0.5f + 0.5f;
	vec2 prevNDC				= (in_PrevClipPosition.xy / in_PrevClipPosition.w) * 0.5f + 0.5f;
	vec2 screenVelocity			= (prevNDC - currentNDC);
	out_Velocity				= vec2(screenVelocity);
}
