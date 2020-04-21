#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location = 0) in flat uint in_InstanceID;

layout(location = 0) out vec4 out_Color;

void main()
{
	uint primID_instanceID = (((gl_PrimitiveID + 1)<< 10) & 0xFFFFFC00) | (in_InstanceID & 0x000003FF);
	out_Color = unpackUnorm4x8(primID_instanceID);
}