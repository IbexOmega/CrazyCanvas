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
layout(location = 9) in vec3		in_VertexColor;

layout(binding = 0, set = BUFFER_SET_INDEX) uniform PerFrameBuffer
{
	SPerFrameBuffer Val;
} u_PerFrameBuffer;

layout(binding = 1, set = BUFFER_SET_INDEX) readonly buffer MaterialParameters 
{
	SMaterialParameters val[]; 
} b_MaterialParameters;

layout(binding = 0, set = TEXTURE_SET_INDEX) uniform sampler2D u_AlbedoMaps[];
layout(binding = 1, set = TEXTURE_SET_INDEX) uniform sampler2D u_NormalMaps[];
layout(binding = 2, set = TEXTURE_SET_INDEX) uniform sampler2D u_CombinedMaterialMaps[];

layout(location = 0) out vec4 out_Albedo;
layout(location = 1) out vec4 out_AO_Rough_Metal_Valid;
layout(location = 2) out vec3 out_Compact_Normal;
layout(location = 3) out vec4 out_Velocity_fWidth_Normal;
layout(location = 4) out vec2 out_Geometric_Normal;

layout (constant_id = 0) const int DEBUG_COLORS = 0;

void main()
{
	vec3 normal		= normalize(in_Normal);
	vec3 tangent	= normalize(in_Tangent);
	vec3 bitangent	= normalize(in_Bitangent);
	vec2 texCoord	= in_TexCoord;

	mat3 TBN = mat3(tangent, bitangent, normal);

	vec4 sampledAlbedo				= texture(u_AlbedoMaps[in_MaterialSlot],			texCoord);
	vec3 sampledNormal				= texture(u_NormalMaps[in_MaterialSlot],			texCoord).rgb;
	vec3 sampledCombinedMaterial	= texture(u_CombinedMaterialMaps[in_MaterialSlot],	texCoord).rgb;

	SMaterialParameters materialParameters = b_MaterialParameters.val[in_MaterialSlot];

	//0
	if (DEBUG_COLORS == 0)
	{
		vec3 storedAlbedo	= pow(materialParameters.Albedo.rgb * sampledAlbedo.rgb, vec3(GAMMA));
		out_Albedo			= vec4(storedAlbedo, sampledAlbedo.a);
	}
	else
	{
		out_Albedo = vec4(in_VertexColor, sampledAlbedo.a);
	}


	//1
	vec3 storedMaterial			= vec3(
									materialParameters.AO * sampledCombinedMaterial.r, 
									materialParameters.Roughness * sampledCombinedMaterial.g, 
									materialParameters.Metallic * sampledCombinedMaterial.b);
	out_AO_Rough_Metal_Valid	= vec4(storedMaterial, 1.0f);

	//2
	vec3 shadingNormal			= normalize((sampledNormal * 2.0f) - 1.0f);
	shadingNormal				= normalize(TBN * normalize(shadingNormal));
	out_Compact_Normal			= PackNormal(shadingNormal);
	
	//3
	const vec2 size		= u_PerFrameBuffer.Val.ViewPortSize;
	const vec2 jitter	= u_PerFrameBuffer.Val.Jitter;
	
	vec2 currentScreenSpace = in_ClipPosition.xy / in_ClipPosition.w;
	currentScreenSpace = (currentScreenSpace * vec2(0.5f, -0.5f)) + 0.5f;
	currentScreenSpace = currentScreenSpace * size;

	vec2 prevScreenSpace = in_PrevClipPosition.xy / in_PrevClipPosition.w;
	prevScreenSpace = (prevScreenSpace * vec2(0.5f, -0.5f)) + 0.5f;
	prevScreenSpace = prevScreenSpace * size;

	vec2 screenVelocity	= currentScreenSpace - prevScreenSpace;
	screenVelocity = screenVelocity - jitter;
	screenVelocity = screenVelocity / size;
	
	float fwidthNorm			= length(fwidth(normal));
	out_Velocity_fWidth_Normal	= vec4(screenVelocity, fwidthNorm, 0.0f);

	//4
	out_Geometric_Normal		= vec2(DirToOct(normal));
}
