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

// Sample Offsets
const int NUM_OFFSETS = 4;
const ivec2 OFFSETS[NUM_OFFSETS] = 
{
	ivec2(0, 0),
	ivec2(0, 1),
	ivec2(1, 0),
	ivec2(1, 1),
};

void main()
{
	// RandomSeed
	const int randomSeed0	= int(u_PerFrameBuffer.Val.RandomSeed);
	const int randomSeed1	= randomSeed0 + 1;
	const int offsetIndex0	= randomSeed0 % NUM_OFFSETS;
	const int offsetIndex1	= randomSeed1 % NUM_OFFSETS;
	const ivec2 offset0		= OFFSETS[offsetIndex0];
	const ivec2 offset1		= OFFSETS[offsetIndex1];

	// This frame's data
	const ivec2	size = textureSize(u_IntermediateOutput, 0);
	const vec2	texCoordOffset = vec2(offset0) / vec2(size);
	vec4 currentSample	= texture(u_IntermediateOutput, in_TexCoord + texCoordOffset);

	// Read HistoryBuffer
	ivec2 historyCoord	= ivec2((in_TexCoord * vec2(size)));
	vec4 previousSample = imageLoad(u_HistoryBuffer, historyCoord + offset1);

	// Calculate sum
	vec4 sum = (previousSample * 0.1f) + (currentSample * 0.9f);

	// Store history and output
	imageStore(u_HistoryBuffer, historyCoord, sum);
	out_Color = sum;
}