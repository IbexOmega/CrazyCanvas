#version 460
#extension GL_GOOGLE_include_directive : enable
#extension GL_ARB_separate_shader_objects : enable

#include "../Defines.glsl"
#include "../Helpers.glsl"

layout(binding = 0, set = TEXTURE_SET_INDEX) uniform sampler2D u_IntermediateOutput;
layout(binding = 1, set = TEXTURE_SET_INDEX) uniform sampler2D u_Velocity;
layout(binding = 2, set = TEXTURE_SET_INDEX) uniform sampler2D u_Depth;
layout(binding = 3, set = TEXTURE_SET_INDEX) uniform sampler2D u_HistoryBuffer;

layout(location = 0) in vec2 in_TexCoord;
layout(location = 0) out vec4 out_Color;

layout(binding = 0, set = BUFFER_SET_INDEX) uniform PerFrameBuffer 
{
	SPerFrameBuffer Val; 
} u_PerFrameBuffer;

#define TAA_HISTORY_DECAY			0.95f
#define TAA_MOTION_AMPLIFICATION	60.0f
#define TAA_CLIP_MODE				1
#define TAA_SEPERATE_NEIGHBORHOOD	0

#if (TAA_SEPERATE_NEIGHBORHOOD == 0)
	#define TAA_BOX_NEIGHBORHOOD	1
	#define TAA_CROSS_NEIGHBORHOOD	1
#else
	#define TAA_BOX_NEIGHBORHOOD	0
	#define TAA_CROSS_NEIGHBORHOOD	1
#endif

// Clip towards center of AABB (Faster version)
bool clipped = false;

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
		clipped = true;
		return pClip + vClip / maUnit;
	}
	else
	{
		return prevSample;
	}
}

// Helper function when dilating the depth buffer
vec2 FrontMostNeigbourTexCoord(vec2 texCoord)
{
	const vec2 size			= u_PerFrameBuffer.Val.ViewPortSize;
	const vec2 pixelSize	= 1.0f / size;
	const float offset		= 2.0f; 

	float samp[5];
	samp[0] = texture(u_Depth, texCoord).r;
	samp[1] = texture(u_Depth, texCoord + (pixelSize * vec2(-offset,-offset))).r;
	samp[2] = texture(u_Depth, texCoord + (pixelSize * vec2( offset,-offset))).r;
	samp[3] = texture(u_Depth, texCoord + (pixelSize * vec2(-offset, offset))).r;
	samp[4] = texture(u_Depth, texCoord + (pixelSize * vec2( offset, offset))).r;
	
	int neighbour = 0;
	float minSamp = samp[0];
	for (int i = 0; i < 5; i++)
	{
		if (samp[i] < minSamp)
		{
			minSamp = samp[i];
			neighbour = i;
		}
	}
	
	switch (neighbour)
	{
	case 0:
		return texCoord;
	case 1:
		return texCoord + pixelSize * vec2(-offset,-offset);
	case 2:
		return texCoord + pixelSize * vec2( offset,-offset);
	case 3:
		return texCoord + pixelSize * vec2(-offset, offset);
	case 4:
		return texCoord + pixelSize * vec2( offset, offset);
  }
}

void main()
{
	// Texcoord
	const vec2 texcoord = in_TexCoord;

	// Disable or enable velocity buffer dilation
#if 1
	vec2 bestTexCoord = FrontMostNeigbourTexCoord(texcoord);
#else
	vec2 bestTexCoord = texcoord;
#endif
	vec2 velocity = texture(u_Velocity, bestTexCoord).xy;

	// This frame's data
	const vec2 size			= u_PerFrameBuffer.Val.ViewPortSize;
	const vec2 pixelSize	= 1.0f / size;
	const vec2 offset		= pixelSize * vec2(2.0f);

	vec4 currentSample = texture(u_IntermediateOutput, texcoord);

#if TAA_BOX_NEIGHBORHOOD
	vec4 samples0[8] = 
	{
		texture(u_IntermediateOutput, texcoord + vec2(-offset.x, -offset.y)),
		texture(u_IntermediateOutput, texcoord + vec2( 0.0,		 -offset.y)),
		texture(u_IntermediateOutput, texcoord + vec2( offset.x, -offset.y)),
		texture(u_IntermediateOutput, texcoord + vec2(-offset.x,  0.0f)),

		texture(u_IntermediateOutput, texcoord + vec2(-offset.x,  offset.y)),
		texture(u_IntermediateOutput, texcoord + vec2( 0.0,		  offset.y)),
		texture(u_IntermediateOutput, texcoord + vec2( offset.x,  offset.y)),
		texture(u_IntermediateOutput, texcoord + vec2( offset.x,  0.0f)),
	};

	vec4 avg0 = 
		samples0[0] +
		samples0[1] +
		samples0[2] +
		samples0[3] +
		samples0[4] +
		samples0[5] +
		samples0[6] +
		samples0[7] +
		currentSample;
	avg0 = avg0 / 9.0f;

	vec4 minSample0 = min(min(min(min(min(min(min(min(samples0[0], samples0[1]), samples0[2]), samples0[3]), samples0[4]), samples0[5]), samples0[6]), samples0[7]), currentSample);
	vec4 maxSample0 = max(max(max(max(max(max(max(max(samples0[0], samples0[1]), samples0[2]), samples0[3]), samples0[4]), samples0[5]), samples0[6]), samples0[7]), currentSample);
#endif

#if TAA_CROSS_NEIGHBORHOOD
	vec4 samples1[4] = 
	{
		texture(u_IntermediateOutput, texcoord + vec2(-offset.x,  0.0f)),
		texture(u_IntermediateOutput, texcoord + vec2( offset.x,  0.0f)),
		texture(u_IntermediateOutput, texcoord + vec2( 0.0f,	  offset.y)),
		texture(u_IntermediateOutput, texcoord + vec2( 0.0f,	 -offset.y)),
	};

	vec4 avg1 = 
		samples1[0] +
		samples1[1] +
		samples1[2] +
		samples1[3] +
		currentSample;
	avg1 = avg1 / 5.0f;

	vec4 minSample1 = min(min(min(min(samples1[0], samples1[1]), samples1[2]), samples1[3]), currentSample);
	vec4 maxSample1 = max(max(max(max(samples1[0], samples1[1]), samples1[2]), samples1[3]), currentSample);
#endif

#if TAA_SEPERATE_NEIGHBORHOOD
	vec4 avg;
	vec4 minSample;
	vec4 maxSample;
	#if TAA_BOX_NEIGHBORHOOD
		avg			= avg0;
		minSample	= minSample0;
		maxSample	= maxSample0;
	#else
		avg			= avg1;
		minSample	= minSample1;
		maxSample	= maxSample1;
	#endif
#else
	vec4 avg		= (avg0 + avg1) * 0.5f;
	vec4 minSample	= (minSample0 + minSample1) * 0.5f;
	vec4 maxSample	= (maxSample0 + maxSample1) * 0.5f;
#endif

	// Read HistoryBuffer
	const vec2 historyTexcoord = texcoord - velocity;
	vec4 historySample = texture(u_HistoryBuffer, historyTexcoord);

	// Clip history sample
#if TAA_CLIP_MODE == 1
	vec3 clippedHistorySample = ClipAABB(minSample.rgb, maxSample.rgb, historySample.rgb, avg.rgb);
#elif TAA_CLIP_MODE == 2
	vec3 clippedHistorySample = clamp(historySample.rgb, minSample.rgb, maxSample.rgb);
#endif

	// Store history
#if 1
	currentSample.a = length(velocity * TAA_MOTION_AMPLIFICATION);
#endif

	// Calculate weights
	const float staticFactor	= 0.98f;
	const float movingFactor	= 0.9f;
#if 0
	float weight = 0.98f;
#elif 1
	float historyWeight = mix(movingFactor, staticFactor, historySample.a);
	float weight		= clamp(historyWeight, movingFactor, staticFactor);
	historySample.rgb = clippedHistorySample;
#elif 1
	historySample.rgb = clippedHistorySample;
	vec2 luma = vec2(CalculateLuminance(currentSample.rgb), CalculateLuminance(historySample.rgb));

	float weight = 1.0f - abs(luma.x - luma.y) / max(luma.x, max(luma.y, 0.2f));
	weight = mix(movingFactor, staticFactor, weight * weight);
#else
	float weight = 0.9f;

	vec4 outColor;
	if (historySample.a == 0.0f) 
	{
		weight = float(clipped);
	}
	else 
	{
		weight = mix(historySample.a, float(clipped), weight);
	}

	historySample.rgb = mix(historySample.rgb, clippedHistorySample, clamp(outColor.a , 0.0f, 1.0f));
#endif

	// Store history and output
	//out_Color = vec4(vec3(velocity * 1000.0, 0.0f), 1.0f);

	currentSample.rgb = mix(currentSample.rgb, historySample.rgb, weight);
	currentSample.a	= currentSample.a * TAA_HISTORY_DECAY;
	//currentSample.a = mix(currentSample.a, historySample.a, weight);
	out_Color = currentSample;
}