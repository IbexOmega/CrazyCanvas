#version 450 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable

#include "../Defines.glsl"

layout(location = 0) in vec2 in_TexCoords;

layout(binding = 0, set = TEXTURE_SET_INDEX) uniform sampler2D u_TextureAtlast;

layout(location = 0) out vec4 out_ParticleImage;

void main()
{
	vec4 color = texture(u_TextureAtlast, in_TexCoords);
	out_ParticleImage = color;
}