#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable

#include "helpers.glsl"

struct SMaterialParameters
{
    vec4    Albedo;
    float   Ambient;
    float   Metallic;
    float   Roughness;
    float   Unreserved;
};

layout(location = 0) in flat uint in_MaterialIndex;
layout(location = 1) in vec3 in_Normal;
layout(location = 2) in vec3 in_Tangent;
layout(location = 3) in vec3 in_Bitangent;
layout(location = 4) in vec2 in_TexCoord;
layout(location = 5) in vec4 in_Position;

layout(binding = 0, set = 0) uniform sampler2D u_SceneAlbedoMaps[1];
layout(binding = 1, set = 0) uniform sampler2D u_SceneNormalMaps[1];
layout(binding = 2, set = 0) uniform sampler2D u_SceneAOMaps[1];
layout(binding = 3, set = 0) uniform sampler2D u_SceneMetallicMaps[1];
layout(binding = 4, set = 0) uniform sampler2D u_SceneRougnessMaps[1];

layout(binding = 5, set = 1) buffer MaterialParameters  	{ SMaterialParameters val[]; }  b_MaterialParameters;

layout(location = 0) out vec4 out_Albedo_AO;
layout(location = 1) out vec4 out_Normals_Metall_Rough;

void main()
{
    vec3 normal 	= normalize(in_Normal);
	vec3 tangent 	= normalize(in_Tangent);
	vec3 bitangent	= normalize(in_Bitangent);
	vec2 texCoord 	= in_TexCoord;

    mat3 TBN = mat3(tangent, bitangent, normal);

	vec3 sampledAlbedo 	    = pow(  texture(u_SceneAlbedoMaps[in_MaterialIndex],      texCoord).rgb, vec3(GAMMA));
	vec3 sampledNormal 	    =       texture(u_SceneNormalMaps[in_MaterialIndex],      texCoord).rgb;
	float sampledAO 		=       texture(u_SceneAOMaps[in_MaterialIndex],          texCoord).r;
	float sampledMetallic 	=       texture(u_SceneMetallicMaps[in_MaterialIndex],    texCoord).r;
	float sampledRoughness  =       texture(u_SceneRougnessMaps[in_MaterialIndex],    texCoord).r;

	sampledNormal 	    = ((sampledNormal * 2.0f) - 1.0f);
	sampledNormal 		= normalize(TBN * normalize(sampledNormal));

	SMaterialParameters materialParameters = b_MaterialParameters.val[in_MaterialIndex];

	//Store normal in 2 component x^2 + y^2 + z^2 = 1, store the sign with roughness
    vec3 storedAlbedo       = materialParameters.Albedo.rgb * sampledAlbedo;
    float storedAO          = materialParameters.Ambient * sampledAO;
	vec2 storedNormal 	    = sampledNormal.xy;
    float storedMetallic    = materialParameters.Metallic * sampledMetallic;
	float storedRoughness    = max(materialParameters.Roughness * sampledRoughness, 0.00001f);
	if (sampledNormal.z < 0)
	{
		storedRoughness = -storedRoughness;
	}

	out_Albedo_AO 				= vec4(storedAlbedo, storedAO);
	out_Normals_Metall_Rough	= vec4(storedNormal, storedMetallic, storedRoughness);
}