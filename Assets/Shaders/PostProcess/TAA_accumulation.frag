#version 460
#extension GL_GOOGLE_include_directive : enable
#extension GL_ARB_separate_shader_objects : enable

#include "../Defines.glsl"
#include "../Helpers.glsl"

layout(binding = 0, set = TEXTURE_SET_INDEX) uniform sampler2D u_IntermediateOutput;
layout(binding = 1, set = TEXTURE_SET_INDEX) uniform sampler2D u_Velocity;
layout(binding = 2, set = TEXTURE_SET_INDEX, rgba8) uniform image2D u_HistoryBuffer;

layout(location = 0) in vec2 in_TexCoord;
layout(location = 0) out vec4 out_Color;

layout(binding = 0, set = BUFFER_SET_INDEX) uniform PerFrameBuffer 
{
	SPerFrameBuffer Val; 
} u_PerFrameBuffer;

// Clip towards center of AABB (Faster version)
vec3 ClipAABB(vec3 aabbMin, vec3 aabbMax, vec3 prevSample, vec3 avg)
{
	vec3 pClip = 0.5f * (aabbMax + aabbMin);
	vec3 eClip = 0.5f * (aabbMax - aabbMin);

	vec3 vClip = prevSample - pClip;
	vec3 vUnit = vClip / eClip;
	vec3 aUnit = abs(vUnit);
	float maUnit = max(aUnit.x, max(aUnit.y, aUnit.z));

	if (maUnit > 1.0)
	{
		return pClip + vClip / maUnit;
	}
	else
	{
		return prevSample;
	}
}

void main()
{
	// Get Size
	const vec2 size = u_PerFrameBuffer.Val.ViewPortSize;
	
	// This frame's data
	const ivec2 texcoord = ivec2(gl_FragCoord.xy);
	vec3 currentSample	= texelFetch(u_IntermediateOutput, texcoord, 0).rgb;

	float avgWeight = 0.0f;
	vec3 avg = vec3(0.0f);
	vec3 minSample = min(currentSample, vec3(1.0f));
	vec3 maxSample = max(currentSample, vec3(0.0f));
	for (int x = -1; x <= 1; x++)
	{
		for (int y = -1; y <= 1; y++)
		{
			ivec2 offset = ivec2(x, y);
			vec3 samp = texelFetch(u_IntermediateOutput, texcoord + offset, 0).rgb;
			minSample = min(samp, minSample);
			maxSample = max(samp, maxSample);

			avg += samp;
			avgWeight += 1.0f;
		}
	}
	avg = avg / avgWeight;

	// Read HistoryBuffer
	const vec2 jitter = u_PerFrameBuffer.Val.Jitter;
	vec2 velocity = texelFetch(u_Velocity, texcoord, 0).xy;
	velocity = (velocity * size);
	
	ivec2 prevTexcoord	= texcoord + ivec2(velocity);
	vec3 previousSample	= imageLoad(u_HistoryBuffer, prevTexcoord).rgb;
	previousSample		= ClipAABB(minSample, maxSample, previousSample, avg);

	// Calculate result with weights (Luminance filtering)
	float prevWeight	= 0.9f;
	float currentWeight	= (1.0f - prevWeight);
#if 1
	prevWeight		= prevWeight * (1.0f / (1.0f + CalculateLuminance(previousSample)));
	currentWeight	= currentWeight * (1.0f / (1.0f + CalculateLuminance(currentSample)));
#endif
	vec3 outColor = (currentSample * currentWeight + previousSample * prevWeight) / (currentWeight + prevWeight);

	// Store history and output
	imageStore(u_HistoryBuffer, prevTexcoord, vec4(outColor, 0.0f));

	// Basically a clear, this is beacuse of the rendergraph
	out_Color = vec4(0.0f);
}