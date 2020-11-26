#version 450 core
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_shader_explicit_arithmetic_types : enable

#define NO_BUFFERS

#include "../Defines.glsl"

layout(binding = 0, set = TEXTURE_SET_INDEX) uniform sampler2D u_ParticleImage;

layout(location = 0) in vec2 in_TexCoord;

layout(location = 0) out vec4 out_IntermediateImage;

void main()
{
	vec3 particlePixel = texture(u_ParticleImage, in_TexCoord).xyz;

	out_IntermediateImage.xyz = particlePixel.xyz;
}