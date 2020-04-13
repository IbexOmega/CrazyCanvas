#version 450
#extension GL_ARB_separate_shader_objects : enable

struct Vertex
{
	vec4 Position;
	vec4 Normal;
	vec4 Tangent;
	vec4 TexCoord;
};

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

layout(location = 0) out vec3 out_Normal;
layout(location = 1) out vec3 out_Tangent;
layout(location = 2) out vec3 out_Bitangent;
layout(location = 3) out vec2 out_TexCoord;
layout(location = 4) out vec4 out_Position;
layout(location = 5) out vec4 out_PrevPosition;

layout (push_constant) uniform Constants
{
	int MaterialIndex;
	int TransformsIndex;
} constants;

layout (binding = 0) uniform PerFrameBuffer
{
	mat4 Projection;
	mat4 View;
	mat4 LastProjection;
	mat4 LastView;
	mat4 InvView;
	mat4 InvProjection;
	vec4 Position;
	vec4 Right;
	vec4 Up;
} g_PerFrame;

layout(binding = 1) buffer vertexBuffer
{
	Vertex vertices[];
};

layout(binding = 7, set = 0) buffer CombinedMaterialParameters
{
	MaterialParameters mp[];
} u_MaterialParameters;

layout(binding = 8, set = 0) buffer CombinedInstanceTransforms
{
	InstanceTransforms t[];
} u_Transforms;

void main()
{
	mat4 currTransform = u_Transforms.t[constants.TransformsIndex].CurrTransform;
	mat4 prevTransform = u_Transforms.t[constants.TransformsIndex].PrevTransform;

	vec3 position 				= vertices[gl_VertexIndex].Position.xyz;
    vec3 normal 				= vertices[gl_VertexIndex].Normal.xyz;
	vec3 tangent 				= vertices[gl_VertexIndex].Tangent.xyz;
	vec4 worldPosition 			= currTransform * vec4(position, 1.0);
	vec4 prevWorldPosition 		= prevTransform * vec4(position, 1.0);

	normal 	= normalize((currTransform * vec4(normal, 0.0)).xyz);
	tangent = normalize((currTransform * vec4(tangent, 0.0)).xyz);

	vec3 bitangent 	= normalize(cross(normal, tangent));
	vec2 texCoord 	= vertices[gl_VertexIndex].TexCoord.xy;

	vec4 viewPosition 		= g_PerFrame.View 		* worldPosition;
	vec4 prevViewPosition 	= g_PerFrame.LastView 	* prevWorldPosition;

	float distance 		= length(viewPosition.xyz);
	float prevDistance 	= length(prevViewPosition.xyz);

	vec4 projPosition 		=  g_PerFrame.Projection 		* viewPosition;
	vec4 prevProjPosition 	=  g_PerFrame.LastProjection 	* prevViewPosition;
	gl_Position = projPosition;

	projPosition.z 		= distance;
	prevProjPosition.z 	= prevDistance;

	out_Normal 			= normal;
	out_Tangent 		= tangent;
	out_Bitangent 		= bitangent;
	out_TexCoord 		= texCoord;
	out_Position		= projPosition;
	out_PrevPosition	= prevProjPosition;
}