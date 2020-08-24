#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable

#include "Helpers.glsl"
#include "Defines.glsl"

layout(location = 0) in flat uint in_MaterialIndex;
layout(location = 1) in vec3 in_Normal;
layout(location = 2) in vec3 in_Tangent;
layout(location = 3) in vec3 in_Bitangent;
layout(location = 4) in vec3 in_LocalNormal;
layout(location = 5) in vec2 in_TexCoord;
layout(location = 6) in vec4 in_WorldPosition;
layout(location = 7) in vec4 in_ClipPosition;
layout(location = 8) in vec4 in_PrevClipPosition;

layout(binding = 6, set = BUFFER_SET_INDEX) restrict readonly buffer MaterialParameters  	{ SMaterialParameters val[]; }  b_MaterialParameters;

layout(binding = 0, set = TEXTURE_SET_INDEX) uniform sampler2D u_SceneAlbedoMaps[MAX_UNIQUE_MATERIALS];
layout(binding = 1, set = TEXTURE_SET_INDEX) uniform sampler2D u_SceneNormalMaps[MAX_UNIQUE_MATERIALS];
layout(binding = 2, set = TEXTURE_SET_INDEX) uniform sampler2D u_SceneAOMaps[MAX_UNIQUE_MATERIALS];
layout(binding = 3, set = TEXTURE_SET_INDEX) uniform sampler2D u_SceneMetallicMaps[MAX_UNIQUE_MATERIALS];
layout(binding = 4, set = TEXTURE_SET_INDEX) uniform sampler2D u_SceneRougnessMaps[MAX_UNIQUE_MATERIALS];

layout(location = 0) out vec4 out_Albedo_AO;
layout(location = 1) out vec4 out_Normals_Metall_Rough;

void main()
{
    vec3 normal 	= normalize(in_Normal);
	vec3 tangent 	= normalize(in_Tangent);
	vec3 bitangent	= normalize(in_Bitangent);
	vec2 texCoord 	= in_TexCoord;

    mat3 TBN = mat3(tangent, bitangent, normal);

	vec3 sampledAlbedo 	    = 		texture(u_SceneAlbedoMaps[in_MaterialIndex],      texCoord).rgb;
	vec3 sampledNormal 	    =       texture(u_SceneNormalMaps[in_MaterialIndex],      texCoord).rgb;
	float sampledAO 		=       texture(u_SceneAOMaps[in_MaterialIndex],          texCoord).r;
	float sampledMetallic 	=       texture(u_SceneMetallicMaps[in_MaterialIndex],    texCoord).r;
	float sampledRoughness  =       texture(u_SceneRougnessMaps[in_MaterialIndex],    texCoord).r;
	
	vec3 worldNormal 	   	= normalize((sampledNormal * 2.0f) - 1.0f);
	worldNormal 			= normalize(TBN * normalize(worldNormal));

	SMaterialParameters materialParameters = b_MaterialParameters.val[in_MaterialIndex];

	//Store normal in 2 component x^2 + y^2 + z^2 = 1, store the sign with roughness
    vec3 storedAlbedo       = pow(materialParameters.Albedo.rgb * sampledAlbedo, vec3(GAMMA));
    float storedAO          = materialParameters.Ambient * sampledAO;
	vec2 storedNormal 	    = worldNormal.xy;
    float storedMetallic    = max(materialParameters.Metallic * sampledMetallic, EPSILON);
	if ((materialParameters.Reserved_Emissive & 0x1) == 0)
	{
		storedMetallic = -storedMetallic;
	}

	float storedRoughness   = max(materialParameters.Roughness * sampledRoughness, EPSILON);
	if (worldNormal.z < 0)
	{
		storedRoughness = -storedRoughness;
	}

	out_Albedo_AO 				= vec4(storedAlbedo, storedAO);
	out_Normals_Metall_Rough	= vec4(storedNormal, storedMetallic, storedRoughness);
}