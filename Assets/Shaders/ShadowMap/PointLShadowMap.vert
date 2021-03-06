#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shader_draw_parameters : enable
#extension GL_GOOGLE_include_directive : enable

#define NO_TEXTURES

#include "../Defines.glsl"

layout( push_constant ) uniform PushConstants {
	uint Iteration;
	uint PointLightIndex;
} pc;

layout(binding = 0, set = 0) restrict readonly buffer LightsBuffer
{
	SLightsBuffer val; 
	SPointLight pointLights[];  
} b_LightsBuffer;

layout(binding = 0, set = 1) restrict readonly buffer Vertices         { SVertex val[]; }       b_Vertices;
layout(binding = 1, set = 1) restrict readonly buffer Instances        { SInstance val[]; }     b_Instances;
layout(binding = 2, set = 1) restrict readonly buffer Meshlets			{ SMeshlet Val[]; } 	b_Meshlets;
layout(binding = 3, set = 1) restrict readonly buffer UniqueIndices	{ uint Val[]; } 		    b_UniqueIndices;
layout(binding = 4, set = 1) restrict readonly buffer PrimitiveIndices	{ uint Val[]; } 		b_PrimitiveIndices;

layout(location = 0) out vec4 out_WorldPos;
layout(location = 1) out flat vec3 out_PointLightPosition;
layout(location = 2) out flat float out_FarPlane;

void main()
{
	SVertex vertex                              = b_Vertices.val[gl_VertexIndex];
	SInstance instance                          = b_Instances.val[gl_InstanceIndex];
	SLightsBuffer lightsBuffer                  = b_LightsBuffer.val;
	
	uint faceIndex          = pc.Iteration % 6U;
	SPointLight pointLight = b_LightsBuffer.pointLights[pc.PointLightIndex];

	out_FarPlane            = pointLight.FarPlane;
	out_WorldPos            = instance.Transform * vec4(vertex.Position.xyz, 1.0f);
	out_PointLightPosition  = pointLight.Position;
	gl_Position             = pointLight.ProjView[faceIndex] * out_WorldPos;
	gl_Position.y           *= -1.0;
}