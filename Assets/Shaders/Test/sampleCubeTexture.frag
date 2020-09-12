#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable

#include "..\Helpers.glsl"
#include "..\Defines.glsl"

layout(location = 0) in vec3 in_VertexPos;

layout(binding = 0, set = TEXTURE_SET_INDEX) uniform samplerCube u_SamplerCubeMap;

layout(location = 0) out vec4 out_Color;

void main()
{
	out_Color 				            = texture(u_SamplerCubeMap, normalize(in_VertexPos));
}