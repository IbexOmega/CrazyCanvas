#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable

#include "Helpers.glsl"
#include "Defines.glsl"

layout(location = 0) in flat uint in_MaterialIndex;
layout(location = 1) in vec3 in_Normal;
layout(location = 2) in vec3 in_Tangent;
layout(location = 3) in vec3 in_Bitangent;
layout(location = 4) in vec2 in_TexCoord;
layout(location = 5) in vec4 in_Position;

layout(binding = 0, set = TEXTURE_SET_INDEX) uniform sampler2D u_SceneAlbedoMaps[MAX_UNIQUE_MATERIALS];

layout(location = 0) out vec4 outColor;

void main()
{
    vec3 normal 	= normalize(in_Normal);
	vec3 tangent 	= normalize(in_Tangent);
	vec3 bitangent	= normalize(in_Bitangent);
	vec2 texCoord 	= in_TexCoord;

    mat3 TBN = mat3(tangent, bitangent, normal);

	vec3 sampledAlbedo 	    = pow(  texture(u_SceneAlbedoMaps[in_MaterialIndex],      texCoord).rgb, vec3(GAMMA));
	outColor 				= vec4(sampledAlbedo, 1.0f);
}