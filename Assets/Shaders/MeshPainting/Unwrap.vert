#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shader_draw_parameters : enable
#extension GL_GOOGLE_include_directive : enable

#include "../Defines.glsl"

layout(push_constant) uniform TransformBuffer
{
	layout(offset = 0) uint Index;
	layout(offset = 4) uint TeamMode;
} p_InstanceInfo;

layout(binding = 0, set = BUFFER_SET_INDEX) uniform PerFrameBuffer				{ SPerFrameBuffer val; }	u_PerFrameBuffer;

layout(binding = 0, set = DRAW_SET_INDEX) restrict readonly buffer Vertices		{ SVertex val[]; }			b_Vertices;
layout(binding = 1, set = DRAW_SET_INDEX) restrict readonly buffer Instances	{ SInstance val[]; }		b_Instances;

layout(binding = 0, set = UNWRAP_DRAW_SET_INDEX) uniform UnwrapData				{ SUnwrapData val; }		u_UnwrapData;

layout(location = 0) out vec3 out_WorldPosition;
layout(location = 1) out vec3 out_Normal;

layout(location = 2) out vec3 out_TargetPosition;
layout(location = 3) out vec3 out_TargetDirection;
layout(location = 4) out flat int out_InstanceTeam;

/*
	Three parameters are needed to make this work. These are:
		- Target position. This is the center of the mask which will be painted. The position is in world space.
		- Direction. This is the direction of the projection onto the mesh, this too is in world space.
		- Brush size. This is in normal world units. (Example: 1 meter)
*/

void main()
{
	SVertex vertex					= b_Vertices.val[gl_VertexIndex];
	mat4 transform					= b_Instances.val[p_InstanceInfo.Index].Transform;
	SPerFrameBuffer perFrameBuffer	= u_PerFrameBuffer.val;

	vec4 worldPosition				= transform * vec4(vertex.Position.xyz, 1.0f);
	vec3 normal						= normalize((transform * vec4(vertex.Normal.xyz, 0.0f)).xyz);

	out_WorldPosition				= worldPosition.xyz;
	out_Normal						= normal;

	out_TargetDirection				= u_UnwrapData.val.TargetDirectionXYZAngleW.xyz;
	out_TargetPosition				= u_UnwrapData.val.TargetPosition.xyz;
	out_InstanceTeam				= int(p_InstanceInfo.TeamMode);

	// This is not removed because of debug purposes.
	//out_TargetDirection             = -normalize(vec3(-perFrameBuffer.View[0][2], -perFrameBuffer.View[1][2], -perFrameBuffer.View[2][2]));
	//out_TargetPosition              = perFrameBuffer.CameraPosition.xyz;  

	vec2 texCoord = vec2(vertex.TexCoord.x, vertex.TexCoord.y);
	texCoord.y = 1.0f - texCoord.y;
	texCoord = (texCoord * 2.0f - 1.0f);
	gl_Position = vec4(vec3(texCoord, 0.0f), 1.0f);
}