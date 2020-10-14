#version 450 core
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_shader_explicit_arithmetic_types : enable

#include "../Defines.glsl"

layout(binding = 0, set = NO_BUFFERS_TEXTURE_SET_INDEX) uniform sampler2D u_ParticleImage;
layout(binding = 1, set = NO_BUFFERS_TEXTURE_SET_INDEX) uniform sampler2D u_ParticleDepth;
layout(binding = 2, set = NO_BUFFERS_TEXTURE_SET_INDEX) uniform sampler2D u_GBufferDepth;

layout(location = 0) in vec2 in_TexCoord;

layout(location = 0) out vec4 finalColor;

void main()
{
	vec4 particlePixel = texture(u_ParticleImage, in_TexCoord);
	float particleDepth = texture(u_ParticleDepth, in_TexCoord).r;
	float bufferDepth = texture(u_GBufferDepth, in_TexCoord).r;

	if (bufferDepth < particleDepth)
		discard;

	finalColor = particlePixel;
}