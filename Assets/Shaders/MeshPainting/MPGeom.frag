#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shader_draw_parameters : enable
#extension GL_GOOGLE_include_directive : enable

#include "../Defines.glsl"
#include "../Helpers.glsl"

layout(location = 0) in flat uint   in_MaterialSlot;
layout(location = 1) in vec3        in_WorldPosition;
layout(location = 2) in vec3        in_Normal;
layout(location = 3) in vec3        in_Tangent;
layout(location = 4) in vec3        in_Bitangent;
layout(location = 5) in vec2        in_TexCoord;
layout(location = 6) in vec4        in_ClipPosition;
layout(location = 7) in vec4        in_PrevClipPosition;
layout(location = 8) in vec3        in_CameraPosition;
layout(location = 9) in vec3        in_CameraDirection;
layout(location = 10) in vec3       in_CameraViewPosition;

layout(binding = 1, set = BUFFER_SET_INDEX) uniform MaterialParameters  	{ SMaterialParameters val[MAX_UNIQUE_MATERIALS]; }  u_MaterialParameters;

layout(binding = 0, set = TEXTURE_SET_INDEX) uniform sampler2D u_AlbedoMaps[MAX_UNIQUE_MATERIALS];
layout(binding = 1, set = TEXTURE_SET_INDEX) uniform sampler2D u_NormalMaps[MAX_UNIQUE_MATERIALS];
layout(binding = 2, set = TEXTURE_SET_INDEX) uniform sampler2D u_AOMaps[MAX_UNIQUE_MATERIALS];
layout(binding = 3, set = TEXTURE_SET_INDEX) uniform sampler2D u_RoughnessMaps[MAX_UNIQUE_MATERIALS];
layout(binding = 4, set = TEXTURE_SET_INDEX) uniform sampler2D u_MetallicMaps[MAX_UNIQUE_MATERIALS];
layout(binding = 5, set = TEXTURE_SET_INDEX) uniform sampler2D u_UnwrappedTexture;

layout(location = 0) out vec4 out_Position;
layout(location = 1) out vec3 out_Albedo;
layout(location = 2) out vec4 out_AO_Rough_Metal_Valid;
layout(location = 3) out vec2 out_Compact_Normal;
layout(location = 4) out vec2 out_Velocity;
layout(location = 5) out vec4 out_MaskTexture;

void main()
{
    vec3 normal 	= normalize(in_Normal);
	vec3 tangent 	= normalize(in_Tangent);
	vec3 bitangent	= normalize(in_Bitangent);
	vec2 texCoord 	= in_TexCoord;

    mat3 TBN = mat3(tangent, bitangent, normal);

	vec3 sampledAlbedo 	    = texture(u_AlbedoMaps[in_MaterialSlot],      texCoord).rgb;
	vec3 sampledNormal 	    = texture(u_NormalMaps[in_MaterialSlot],      texCoord).rgb;
	float sampledAO 		= texture(u_AOMaps[in_MaterialSlot],          texCoord).r;
	float sampledMetallic 	= texture(u_MetallicMaps[in_MaterialSlot],    texCoord).r;
	float sampledRoughness  = texture(u_RoughnessMaps[in_MaterialSlot],   texCoord).r;
	
	vec3 shadingNormal 	   	= normalize((sampledNormal * 2.0f) - 1.0f);
	shadingNormal 			= normalize(TBN * normalize(shadingNormal));

	SMaterialParameters materialParameters = u_MaterialParameters.val[in_MaterialSlot];

    vec2 currentNDC 	= (in_ClipPosition.xy / in_ClipPosition.w) * 0.5f + 0.5f;
	vec2 prevNDC 		= (in_PrevClipPosition.xy / in_PrevClipPosition.w) * 0.5f + 0.5f;

	//0
	out_Position				= vec4(in_WorldPosition, 0.0f);

    //1
    vec3 storedAlbedo         	= pow(materialParameters.Albedo.rgb * sampledAlbedo, vec3(GAMMA));
	out_Albedo 				  	= storedAlbedo;

    //2
    float storedAO            	= materialParameters.AO * sampledAO;
    float storedRoughness     	= materialParameters.Roughness * sampledRoughness;
    float storedMetallic      	= materialParameters.Metallic * sampledMetallic;
	out_AO_Rough_Metal_Valid	= vec4(storedAO, storedRoughness, storedMetallic, 1.0f);

    //3
	vec2 storedShadingNormal  	= DirToOct(shadingNormal);
	out_Compact_Normal       	= storedShadingNormal;

    //4
	vec2 screenVelocity 	  	= (prevNDC - currentNDC);// + in_CameraJitter;
	out_Velocity              	= vec2(screenVelocity); 

    float depth = in_ClipPosition.z;
    vec3 targetPosition = in_CameraPosition;
    vec3 projectedPosition = targetPosition + in_CameraDirection*depth;
    const float BRUSH_SIZE = 0.5f;

    // 5
    if(length(in_WorldPosition-projectedPosition) <= BRUSH_SIZE)
        out_MaskTexture = vec4(1.f, 1.f, 1.f, 1.f);
    //else
    out_MaskTexture = texture(u_UnwrappedTexture, texCoord);
}
