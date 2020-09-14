#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable

#include "Defines.glsl"

layout(location = 0) in vec3 in_WorldPos;

layout(binding = 0, set = TEXTURE_SET_INDEX) uniform samplerCube u_Skybox;

layout(location = 0) out vec4 out_Color;

void main()
{
	out_Color = texture(u_Skybox, normalize(in_WorldPos));
}