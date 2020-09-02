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

layout(binding = 5, set = BUFFER_SET_INDEX) uniform PerFrameBuffer                          { SPerFrameBuffer val; }        u_PerFrameBuffer;
layout(binding = 6, set = BUFFER_SET_INDEX) restrict readonly buffer MaterialParameters  	{ SMaterialParameters val[]; }  b_MaterialParameters;

layout(binding = 0, set = TEXTURE_SET_INDEX) uniform sampler2D u_SceneAlbedoMaps[MAX_UNIQUE_MATERIALS];
layout(binding = 1, set = TEXTURE_SET_INDEX) uniform sampler2D u_SceneNormalMaps[MAX_UNIQUE_MATERIALS];
layout(binding = 2, set = TEXTURE_SET_INDEX) uniform sampler2D u_SceneAOMaps[MAX_UNIQUE_MATERIALS];
layout(binding = 3, set = TEXTURE_SET_INDEX) uniform sampler2D u_SceneMetallicMaps[MAX_UNIQUE_MATERIALS];
layout(binding = 4, set = TEXTURE_SET_INDEX) uniform sampler2D u_SceneRougnessMaps[MAX_UNIQUE_MATERIALS];

layout(location = 0) out vec4 out_Color;

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
	
	vec3 shadingNormal 	   	= normalize((sampledNormal * 2.0f) - 1.0f);
	shadingNormal 			= normalize(TBN * normalize(shadingNormal));

	SMaterialParameters materialParameters = b_MaterialParameters.val[in_MaterialIndex];

    vec3 albedo                         = pow(materialParameters.Albedo.rgb * sampledAlbedo, vec3(GAMMA));
    
    vec3 lightColor                     = vec3(1.0f);

    vec3 viewDir                        = normalize(u_PerFrameBuffer.val.Position.xyz - in_WorldPosition.xyz);
    vec3 lightDir                       = normalize(vec3(1.0f, 1.0f, 0.0f));
    vec3 reflectionDir                  = reflect(-lightDir, shadingNormal);

    float diff                          = max(dot(shadingNormal, lightDir), 0.0f);
    float spec                          = pow(max(dot(viewDir, reflectionDir), 0.0f), 32);

    vec3 ambient                        = 0.1f * lightColor;
    vec3 diffuse                        = diff * lightColor;
    vec3 specular                       = 0.5f * spec * lightColor;

    vec3 finalColor                     = (ambient + diffuse + specular) * albedo;
    finalColor                          = finalColor / (finalColor + vec3(1.0f));
	finalColor                          = pow(finalColor, vec3(1.0f / GAMMA));

	out_Color 				            = vec4(finalColor, 1.0f);
}