#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shader_draw_parameters : enable
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_nonuniform_qualifier : enable

#include "../Defines.glsl"
#include "../Helpers.glsl"

layout(location = 0) in flat uint	in_MaterialSlot;
layout(location = 1) in vec3		in_WorldPosition;
layout(location = 2) in vec3		in_Normal;
layout(location = 3) in vec3		in_Tangent;
layout(location = 4) in vec3		in_Bitangent;
layout(location = 5) in vec2		in_TexCoord;
layout(location = 6) in vec4		in_ClipPosition;
layout(location = 7) in vec4		in_PrevClipPosition;
layout(location = 8) in flat uint	in_ExtensionIndex;
layout(location = 9) in flat uint	in_InstanceIndex;
layout(location = 10) in vec3 		in_ViewDirection;

layout(push_constant) uniform TeamIndex
{
	uint Index;
} p_TeamIndex;

layout(binding = 1, set = BUFFER_SET_INDEX) readonly buffer MaterialParameters  	{ SMaterialParameters val[]; }	b_MaterialParameters;
layout(binding = 2, set = BUFFER_SET_INDEX) readonly buffer PaintMaskColors			{ vec4 val[]; }					b_PaintMaskColor;

layout(binding = 0, set = TEXTURE_SET_INDEX) uniform sampler2D u_AlbedoMaps[];
layout(binding = 1, set = TEXTURE_SET_INDEX) uniform sampler2D u_NormalMaps[];
layout(binding = 2, set = TEXTURE_SET_INDEX) uniform sampler2D u_CombinedMaterialMaps[];

layout(binding = 0, set = DRAW_EXTENSION_SET_INDEX) uniform sampler2D u_PaintMaskTextures[];

layout(location = 0) out vec4 out_Color;

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

	vec3 shadingNormal		= normalize((sampledNormal * 2.0f) - 1.0f);
	shadingNormal			= normalize(TBN * normalize(shadingNormal));

	SMaterialParameters materialParameters = b_MaterialParameters.val[in_MaterialSlot];

	vec2 currentNDC		= (in_ClipPosition.xy / in_ClipPosition.w) * 0.5f + 0.5f;
	vec2 prevNDC		= (in_PrevClipPosition.xy / in_PrevClipPosition.w) * 0.5f + 0.5f;

	uint serverData				= floatBitsToUint(texture(u_PaintMaskTextures[in_ExtensionIndex], texCoord).r);
	uint clientData				= floatBitsToUint(texture(u_PaintMaskTextures[in_ExtensionIndex], texCoord).g);
	float shouldPaint 			= float((serverData & 0x1) | (clientData & 0x1));

	uint clientTeam				= (clientData >> 1) & 0x7F;
	uint serverTeam				= (serverData >> 1) & 0x7F;
	uint clientPainting			= clientData & 0x1;
	uint team = serverTeam;
	
	if (clientPainting > 0)
		team = clientTeam;

	// TODO: Change this to a buffer input which we can index the team color to
	vec3 color = b_PaintMaskColor.val[team].rgb;

	float backSide = 1.0f - step(0.0f, dot(in_ViewDirection, shadingNormal));
	color = mix(color, color*0.8, backSide);

	// Only render team members and paint on enemy players
	uint enemy = p_TeamIndex.Index;
	bool isPainted = !(shouldPaint < 0.5f);
	if(enemy != 0 && !isPainted)
		discard;

	//1
	vec3 storedAlbedo = pow(materialParameters.Albedo.rgb * sampledAlbedo, vec3(GAMMA));

	// 5	
	out_Color = vec4(mix(storedAlbedo, color, shouldPaint), isPainted ? 1.0f : 0.6f);
}
