#version 460
#extension GL_GOOGLE_include_directive : enable
#extension GL_ARB_separate_shader_objects : enable

#define NO_BUFFERS
#include "../Defines.glsl"

layout(binding = 0, set = TEXTURE_SET_INDEX) uniform sampler2D u_HistoryBuffer;

layout(location = 0) in vec2 in_TexCoord;
layout(location = 0) out vec4 out_Color;

// Kernel
const float KERNEL[9] = 
{
	0.024879f, 0.107973f, 0.024879f,
	0.107973f, 0.468592f, 0.107973f,
	0.024879f, 0.107973f, 0.024879f,
};

void main()
{
	// Get Size
	const vec2 texcoord		= in_TexCoord;
	const vec2 size			= vec2(textureSize(u_HistoryBuffer, 0));
	const vec2 pixelSize	= 1.0f / size;

	// Reconstrucion filter
	vec4 finalSample = vec4(0.0f);
	float totalWeight = 0.0f;

	int index = 0;
	for (int x = -1; x <= 1; x++)
	{
		for (int y = -1; y <= 1; y++)
		{
			const vec2	offset = vec2(x, y) * pixelSize;
			const float weight = KERNEL[index];
			finalSample	+= texture(u_HistoryBuffer, texcoord + offset) * weight;
			totalWeight += weight;

			index++;
		}
	}
	finalSample /= totalWeight;

	out_Color = vec4(texture(u_HistoryBuffer, texcoord));
}