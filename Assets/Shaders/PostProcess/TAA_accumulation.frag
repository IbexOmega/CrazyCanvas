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

#define TAA_HISTORY_DECAY			0.9f
#define TAA_MOTION_AMPLIFICATION	80.0f
#define TAA_CLIP_MODE				2
#define TAA_YCoCg					0

// https://software.intel.com/en-us/node/503873
vec3 RGB_YCoCg(vec3 c)
{
	// Y	= R/4 + G/2 + B/4
	// Co	= R/2 - B/2
	// Cg	= -R/4 + G/2 - B/4
	return vec3(
			 c.x / 4.0f + c.y / 2.0f + c.z / 4.0f,
			 c.x / 2.0f - c.z / 2.0f,
			-c.x / 4.0f + c.y / 2.0f - c.z / 4.0f
	);
}

// https://software.intel.com/en-us/node/503873
vec3 YCoCg_RGB(vec3 c)
{
	// R = Y + Co - Cg
	// G = Y + Cg
	// B = Y - Co - Cg
	return clamp(vec3(
		c.x + c.y - c.z,
		c.x + c.z,
		c.x - c.y - c.z
	), 0.0f, 1.0f);
}

// Clip towards center of AABB (Faster version)
vec3 ClipAABB(vec3 aabbMin, vec3 aabbMax, vec3 p, vec3 q)
{
	const float FLT_EPS = 0.00000001f;

	vec3 pClip = 0.5f * (aabbMax + aabbMin);
	vec3 eClip = 0.5f * (aabbMax - aabbMin) + FLT_EPS;

	vec3 vClip = q - pClip;
	vec3 vUnit = vClip / eClip;
	vec3 aUnit = abs(vUnit);
	float maUnit = max(aUnit.x, max(aUnit.y, aUnit.z));

	if (maUnit > 1.0f)
	{
		return pClip + vClip / maUnit;
	}
	else
	{
		return q;
	}
}

vec4 Sample(in sampler2D tex, vec2 texCoords)
{
	vec4 texel = texture(tex, texCoords);
	// Gamma Correct
#if 1
	texel.rgb = pow(texel.rgb, vec3(GAMMA));
#endif
	// Change color space
#if TAA_YCoCg
	texel.rgb = RGB_YCoCg(texel.rgb);
#endif
	return texel;
}

vec4 Resolve(vec4 color)
{
	// Change color space
#if TAA_YCoCg
	color.rgb = YCoCg_RGB(color.rgb);
#endif

	// Gamma Correct
#if 1
	color.rgb = pow(color.rgb, vec3(1.0f / GAMMA));
#endif
	return color;
}

// Helper function when dilating the depth buffer
#define NUM_SAMPLES 9

vec2 FrontMostNeigbourTexCoord(vec2 texCoord)
{
	const vec2 size			= u_PerFrameBuffer.Val.ViewPortSize;
	const vec2 pixelSize	= 1.0f / size;
	const float offset		= 2.0f; 

	float samp[NUM_SAMPLES];
	samp[0] = texture(u_Depth, texCoord).r;
	samp[1] = texture(u_Depth, texCoord + pixelSize * vec2(-offset,-offset)).r;
	samp[2] = texture(u_Depth, texCoord + pixelSize * vec2( offset,-offset)).r;
	samp[3] = texture(u_Depth, texCoord + pixelSize * vec2(-offset, offset)).r;
	samp[4] = texture(u_Depth, texCoord + pixelSize * vec2( offset, offset)).r;
	samp[5] = texture(u_Depth, texCoord + pixelSize * vec2(-offset, 0.0f)).r;
	samp[6] = texture(u_Depth, texCoord + pixelSize * vec2( 0.0f,  -offset)).r;
	samp[7] = texture(u_Depth, texCoord + pixelSize * vec2( 0.0f,   offset)).r;
	samp[8] = texture(u_Depth, texCoord + pixelSize * vec2( offset, 0.0f)).r;
	
	int neighbour = 0;
	float minSamp = samp[0];
	for (int i = 0; i < NUM_SAMPLES; i++)
	{
		if (samp[i] < minSamp)
		{
			minSamp		= samp[i];
			neighbour	= i;
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
	case 5:
		return texCoord + pixelSize * vec2(-offset, 0.0f);
	case 6:
		return texCoord + pixelSize * vec2( 0.0f,  -offset);
	case 7:
		return texCoord + pixelSize * vec2( 0.0f,   offset);
	case 8:
		return texCoord + pixelSize * vec2( offset, 0.0f);
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
	const vec2 offset		= pixelSize * vec2(1.0f);

	vec4 currentSample = Sample(u_IntermediateOutput, texcoord);

	// Calculate avg, min, and max in neighbourhood
	vec3 samples[8] = 
	{
		Sample(u_IntermediateOutput, texcoord + vec2(-offset.x, -offset.y)).rgb,
		Sample(u_IntermediateOutput, texcoord + vec2( 0.0f,		-offset.y)).rgb,
		Sample(u_IntermediateOutput, texcoord + vec2( offset.x, -offset.y)).rgb,
		Sample(u_IntermediateOutput, texcoord + vec2(-offset.x,  0.0f)).rgb,
		Sample(u_IntermediateOutput, texcoord + vec2(-offset.x,  offset.y)).rgb,
		Sample(u_IntermediateOutput, texcoord + vec2( 0.0f,		 offset.y)).rgb,
		Sample(u_IntermediateOutput, texcoord + vec2( offset.x,  offset.y)).rgb,
		Sample(u_IntermediateOutput, texcoord + vec2( offset.x,  0.0f)).rgb,
	};

	vec3 avg0 = 
		samples[0] +
		samples[1] +
		samples[2] +
		samples[3] +
		samples[4] +
		samples[5] +
		samples[6] +
		samples[7] +
		currentSample.rgb;
	avg0 = avg0 / 9.0f;

	vec3 minSample0 = min(min(min(min(min(min(min(min(samples[0], samples[1]), samples[2]), samples[3]), samples[4]), samples[5]), samples[6]), samples[7]), currentSample.rgb);
	vec3 maxSample0 = max(max(max(max(max(max(max(max(samples[0], samples[1]), samples[2]), samples[3]), samples[4]), samples[5]), samples[6]), samples[7]), currentSample.rgb);

	vec3 avg1 = 
		samples[1] +
		samples[3] +
		samples[5] +
		samples[7] +
		currentSample.rgb;
	avg1 = avg1 / 5.0f;

	vec3 minSample1 = min(min(min(min(samples[1], samples[3]), samples[5]),samples[7]), currentSample.rgb);
	vec3 maxSample1 = max(max(max(max(samples[1], samples[3]), samples[5]),samples[7]), currentSample.rgb);

	vec3 avg		= (avg0 + avg1) * 0.5f;
	vec3 minSample	= (minSample0 + minSample1) * 0.5f;
	vec3 maxSample	= (maxSample0 + maxSample1) * 0.5f;

	// Calculate color bounding box (YCoCg space)
#if TAA_YCoCg
	{
		vec2 chromaExtent = vec2(0.25f * 0.5f * (maxSample.r - minSample.r));
		vec2 chromaCenter = currentSample.gb;
		minSample.yz	= chromaCenter - chromaExtent;
		maxSample.yz	= chromaCenter + chromaExtent;
		avg.yz			= chromaCenter;
	}
#endif

	// Read HistoryBuffer
	const vec2 historyTexcoord = texcoord - velocity;
	vec4 historySample = Sample(u_HistoryBuffer, historyTexcoord);

	// Clip history sample
	vec3 clippedHistorySample;
#if TAA_CLIP_MODE == 1
	{
		clippedHistorySample = ClipAABB(minSample, maxSample, clamp(avg, minSample, maxSample), historySample.rgb);
	}
#elif TAA_CLIP_MODE == 2
	{
		clippedHistorySample = clamp(historySample.rgb, minSample, maxSample);
	}
#endif

	// Calculate weights
	float weight;
#if 0
	{
		const float staticFactor	= 0.97f;
		const float movingFactor	= 0.92f;
		const float historyWeight	= mix(staticFactor, movingFactor, historySample.a);
		
		weight = clamp(historyWeight, movingFactor, staticFactor);
		historySample.rgb = clippedHistorySample;
	}
#elif 1
	{
		const float maxFactor	= 0.97f;
		const float minFactor	= 0.86f;
	#if TAA_YCoCg
		float lum0 = currentSample.r;
		float lum1 = historySample.r;
	#else
		float lum0 = CalculateLuminance(currentSample.rgb);
		float lum1 = CalculateLuminance(historySample.rgb);
	#endif

		float unbiasedDiff		= abs(lum0 - lum1) / max(lum0, max(lum1, 0.1f));
		float unbiasedWeight	= 1.0f - unbiasedDiff;
		float unbiasedWeightSqr = unbiasedWeight * unbiasedWeight;
		weight				= mix(minFactor, maxFactor, unbiasedWeightSqr);
		historySample.rgb	= clippedHistorySample;
	}
#else
	{
		weight = 0.98f;
		historySample.rgb = clippedHistorySample;
	}
#endif

	// Store history
#if 1
	currentSample.a = length(velocity);
#endif

	// Store history and output
	//out_Color = vec4(vec3(velocity * 100.0, 0.0f), 1.0f);

	currentSample = Resolve(mix(currentSample, historySample, weight));

	// Maniplulate history
#if 1
	currentSample.a = currentSample.a * TAA_HISTORY_DECAY;
#else
	currentSample.a = weight;
#endif

	out_Color = currentSample;
}