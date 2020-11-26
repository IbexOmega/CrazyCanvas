#version 460
#extension GL_GOOGLE_include_directive : enable
#extension GL_ARB_separate_shader_objects : enable

#include "../Defines.glsl"

layout(binding = 0, set = TEXTURE_SET_INDEX) uniform sampler2D u_IntermediateOutput;
layout(binding = 1, set = TEXTURE_SET_INDEX) uniform sampler2D u_Velocity;
layout(binding = 2, set = TEXTURE_SET_INDEX, rgba8) uniform image2D u_HistoryBuffer;

layout(location = 0) in vec2 in_TexCoord;
layout(location = 0) out vec4 out_Color;

layout(binding = 0, set = BUFFER_SET_INDEX) uniform PerFrameBuffer 
{
	SPerFrameBuffer Val; 
} u_PerFrameBuffer;

void main()
{
	// Get Size
	const ivec2	size = textureSize(u_IntermediateOutput, 0);
	
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

	// Jitter
	const vec2 jitter = u_PerFrameBuffer.Val.Jitter;

	// Read HistoryBuffer
	vec2 velocity = texture(u_Velocity, texcoord).xy;
	velocity = velocity - jitter;

	vec2 prevTexcoord = (texcoord - velocity);

	ivec2 historyCoord	= ivec2((prevTexcoord * vec2(size)));
	vec4 previousSample	= imageLoad(u_HistoryBuffer, historyCoord);
	previousSample		= clamp(previousSample, minSample, maxSample);

	// Calculate sum
	const float currentWeight = 0.1f;
	vec4 sample1 = (currentSample * currentWeight) + (previousSample * (1.0f - currentWeight));

	// Store history and output
	imageStore(u_HistoryBuffer, historyCoord, sample1);

	// Basically a clear, this is beacuse of the rendergraph
	out_Color = vec4(0.0f);
}