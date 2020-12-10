#version 460
#extension GL_GOOGLE_include_directive : enable
#extension GL_ARB_separate_shader_objects : enable

#include "../Defines.glsl"

layout(binding = 0, set = TEXTURE_SET_INDEX) uniform sampler2D u_IntermediateOutput;
layout(binding = 1, set = TEXTURE_SET_INDEX, rgba8) uniform image2D u_HistoryBuffer;

layout(location = 0) in vec2 in_TexCoord;
layout(location = 0) out vec4 out_Color;

layout(binding = 0, set = BUFFER_SET_INDEX) uniform PerFrameBuffer 
{
	SPerFrameBuffer Val; 
} u_PerFrameBuffer;

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
	const ivec2	size = textureSize(u_IntermediateOutput, 0);
	
	// Jitter
	vec2 jitter = u_PerFrameBuffer.Val.Jitter;

	// This frame's data
	const vec2 texcoord = in_TexCoord;
	vec4 currentSample	= texture(u_IntermediateOutput, texcoord);

	vec4 minSample = min(currentSample, 1.0f);
	vec4 maxSample = max(currentSample, 0.0f);
	for (int x = -1; x <= 1; x++)
	{
		for (int y = -1; y <= 1; y++)
		{
			vec2 offset = vec2(ivec2(x, y)) / vec2(size);
			vec4 samp = texture(u_IntermediateOutput, texcoord + offset);
			minSample = min(samp, minSample);
			maxSample = max(samp, maxSample);
		}
	}

	// Read HistoryBuffer
	ivec2 historyCoord	= ivec2((texcoord * vec2(size)));
	vec4 previousSample	= imageLoad(u_HistoryBuffer, historyCoord);
	//previousSample		= clamp(previousSample, minSample, maxSample);

	// Calculate sum
	const float currentWeight = 0.1f;
	vec4 sample1 = (currentSample * currentWeight) + (previousSample * (1.0f - currentWeight));

	// Store history and output
	imageStore(u_HistoryBuffer, historyCoord, sample1);

	// Reconstrutcion filter
	vec4 finalSample = vec4(0.0f);
	float totalWeight = 0.0f;
	for (int x = -1; x <= 1; x++)
	{
		for (int y = -1; y <= 1; y++)
		{
			ivec2 offset = ivec2(x, y);
			float weight = KERNEL[y * 3 + x];
			finalSample	+= imageLoad(u_HistoryBuffer, historyCoord + offset) * weight;
			totalWeight += weight;
		}
	}
	finalSample /= totalWeight;
	out_Color = finalSample;
}