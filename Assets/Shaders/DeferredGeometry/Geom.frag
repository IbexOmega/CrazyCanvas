#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shader_draw_parameters : enable
#extension GL_GOOGLE_include_directive : enable

#include "../Defines.glsl"
#include "../Helpers.glsl"

layout(location = 0) in flat uint   in_MaterialIndex;
layout(location = 1) in vec3        in_Normal;
layout(location = 2) in vec3        in_Tangent;
layout(location = 3) in vec3        in_Bitangent;
layout(location = 4) in vec2        in_TexCoord;
layout(location = 5) in vec4        in_ClipPosition;
layout(location = 6) in vec4        in_PrevClipPosition;

layout(binding = 6, set = BUFFER_SET_INDEX) uniform MaterialParameters  	{ SMaterialParameters val[MAX_UNIQUE_MATERIALS]; }  u_MaterialParameters;

layout(binding = 0, set = TEXTURE_SET_INDEX) uniform sampler2D u_SceneAlbedoMaps[MAX_UNIQUE_MATERIALS];
layout(binding = 1, set = TEXTURE_SET_INDEX) uniform sampler2D u_SceneNormalMaps[MAX_UNIQUE_MATERIALS];
layout(binding = 2, set = TEXTURE_SET_INDEX) uniform sampler2D u_SceneAOMaps[MAX_UNIQUE_MATERIALS];
layout(binding = 4, set = TEXTURE_SET_INDEX) uniform sampler2D u_SceneRoughnessMaps[MAX_UNIQUE_MATERIALS];
layout(binding = 3, set = TEXTURE_SET_INDEX) uniform sampler2D u_SceneMetallicMaps[MAX_UNIQUE_MATERIALS];

layout(location = 0) out vec3 out_Albedo;
layout(location = 1) out vec3 out_AO_Rough_Metal;
layout(location = 2) out vec4 out_Compact_Normals;
layout(location = 3) out vec2 out_Velocity;

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
	float sampledRoughness  =       texture(u_SceneRoughnessMaps[in_MaterialIndex],   texCoord).r;
	
	vec3 shadingNormal 	   	= normalize((sampledNormal * 2.0f) - 1.0f);
	shadingNormal 			= normalize(TBN * normalize(shadingNormal));

	SMaterialParameters materialParameters = u_MaterialParameters.val[in_MaterialIndex];

    vec2 currentNDC 	= (in_ClipPosition.xy / in_ClipPosition.w) * 0.5f + 0.5f;
	vec2 prevNDC 		= (in_PrevClipPosition.xy / in_PrevClipPosition.w) * 0.5f + 0.5f;

    //0
    vec3 storedAlbedo         	= pow(materialParameters.Albedo.rgb * sampledAlbedo, vec3(GAMMA));

    //1
    float storedAO            	= materialParameters.Ambient * sampledAO;
    float storedRoughness     	= materialParameters.Roughness * sampledRoughness;
    float storedMetallic      	= materialParameters.Metallic * sampledMetallic;

    //2
	vec2 storedShadingNormal  	= DirToOct(shadingNormal);
    vec2 storedGeometricNormal	= DirToOct(in_Normal);
    
    //3
	vec2 screenVelocity 	  	= (prevNDC - currentNDC);// + in_CameraJitter;

	out_Albedo 				  	= vec3(storedAlbedo);
	out_AO_Rough_Metal	      	= vec3(storedAO, storedRoughness, storedMetallic);
    out_Compact_Normals       	= vec4(storedShadingNormal, storedGeometricNormal);
    out_Velocity              	= vec2(screenVelocity);
}
