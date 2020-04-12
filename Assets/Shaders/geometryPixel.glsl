#version 450
#extension GL_ARB_separate_shader_objects : enable

struct MaterialParameters
{
	vec4 Albedo;
	float Metallic;
	float Roughness;
	float AO;
	float Padding;
};

struct InstanceTransforms
{
	mat4 CurrTransform;
	mat4 PrevTransform;
};

layout(location = 0) in vec3 in_Normal;
layout(location = 1) in vec3 in_Tangent;
layout(location = 2) in vec3 in_Bitangent;
layout(location = 3) in vec2 in_TexCoord;
layout(location = 4) in vec4 in_Position;
layout(location = 5) in vec4 in_PrevPosition;

layout(location = 0) out vec4 out_Albedo_AO;
layout(location = 1) out vec4 out_Normals_Metall_Rough;
layout(location = 2) out vec4 out_Velocity;

layout (push_constant) uniform Constants
{
	int MaterialIndex;
	int TransformsIndex;
} constants;

layout(binding = 2) uniform sampler2D u_AlbedoMap;
layout(binding = 3) uniform sampler2D u_NormalMap;
layout(binding = 4) uniform sampler2D u_AmbientOcclusionMap;
layout(binding = 5) uniform sampler2D u_MetallicMap;
layout(binding = 6) uniform sampler2D u_RoughnessMap;

layout(binding = 7, set = 0) buffer CombinedMaterialParameters
{
	MaterialParameters mp[];
} u_MaterialParameters;

layout(binding = 8, set = 0) buffer CombinedInstanceTransforms
{
	InstanceTransforms t[];
} u_Transforms;

const float GAMMA = 2.2f;

void main()
{
	vec3 normal 	= normalize(in_Normal);
	vec3 tangent 	= normalize(in_Tangent);
	vec3 bitangent	= normalize(in_Bitangent);
	vec2 texcoord 	= in_TexCoord;

	mat3 tbn = mat3(tangent, bitangent, normal);

	vec3 texColor 	= pow(texture(u_AlbedoMap, texcoord).rgb, vec3(GAMMA));
	vec3 normalMap 	= texture(u_NormalMap, texcoord).rgb;
	float ao 		= texture(u_AmbientOcclusionMap, texcoord).r;
	float metallic 	= texture(u_MetallicMap, texcoord).r;
	float roughness = texture(u_RoughnessMap, texcoord).r;

	vec3 sampledNormal 	= ((normalMap * 2.0f) - 1.0f);
	sampledNormal 		= normalize(tbn * normalize(sampledNormal));

	MaterialParameters materialParameters = u_MaterialParameters.mp[constants.MaterialIndex];

	//Store normal in 2 component x^2 + y^2 + z^2 = 1, store the sign with roughness
	vec2 storedNormal 	= sampledNormal.xy;
	roughness 			= max(materialParameters.Roughness * roughness, 0.00001f);
	if (sampledNormal.z < 0)
	{
		roughness = -roughness;
	}

	vec2 a 			= (in_Position.xy 		/ in_Position.w) 		* 0.5f + 0.5f;
	vec2 b 			= (in_PrevPosition.xy 	/ in_PrevPosition.w) 	* 0.5f + 0.5f;
	vec2 velocity 	= b - a;
	float distance 	= in_PrevPosition.z - in_Position.z;

	out_Albedo_AO 				= vec4(materialParameters.Albedo.rgb * texColor, materialParameters.AO * ao);
	out_Normals_Metall_Rough	= vec4(storedNormal, materialParameters.Metallic * metallic, roughness);
	out_Velocity				= vec4(velocity, distance, 1.0f);
}