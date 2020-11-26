#version 460
#extension GL_GOOGLE_include_directive : enable
#extension GL_ARB_separate_shader_objects : enable

#include "../Defines.glsl"
#include "../Helpers.glsl"

layout(binding = 0, set = TEXTURE_SET_INDEX) uniform sampler2D u_IntermediateOutput;
layout(binding = 1, set = TEXTURE_SET_INDEX) uniform sampler2D u_Velocity;
layout(binding = 2, set = TEXTURE_SET_INDEX) uniform sampler2D u_Depth;
layout(binding = 3, set = TEXTURE_SET_INDEX, rgba8) uniform image2D u_HistoryBuffer;

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

// Helper function when dilating the depth buffer
vec2 FrontMostNeigbourTexCoord(vec2 texCoord)
{
	const vec2 size			= u_PerFrameBuffer.Val.ViewPortSize;
	const vec2 pixelSize	= 1.0f / size;

	float samp[5];
	samp[0] = textureOffset(u_Depth, texCoord, ivec2( 0, 0)).r;
	samp[1] = textureOffset(u_Depth, texCoord, ivec2(-2,-2)).r;
	samp[2] = textureOffset(u_Depth, texCoord, ivec2( 2,-2)).r;
	samp[3] = textureOffset(u_Depth, texCoord, ivec2(-2, 2)).r;
	samp[4] = textureOffset(u_Depth, texCoord, ivec2( 2, 2)).r;
	
	int neighbour = 0;
	float minSamp = samp[0];
	for (int i = 0; i < 9; i++)
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
		return texCoord - pixelSize * vec2( 2.0f, 2.0f);
	case 2:
		return texCoord - pixelSize * vec2(-2.0f, 2.0f);
	case 3:
		return texCoord - pixelSize * vec2( 2.0f,-2.0f);
	case 4:
		return texCoord - pixelSize * vec2(-2.0f,-2.0f);
  }
}

void main()
{
	// Get Size
	const vec2 size = u_PerFrameBuffer.Val.ViewPortSize;
	
	// This frame's data
	const vec2 texcoord = vec2(gl_FragCoord.xy);
	const vec2 texUV	= texcoord / size;
	vec3 currentSample	= texture(u_IntermediateOutput, texUV).rgb;

	float avgWeight = 0.0f;
	vec3 avg = vec3(0.0f);
	vec3 minSample = min(currentSample, vec3(9999999.0f));
	vec3 maxSample = max(currentSample, vec3(-9999999.0f));
	for (int x = -1; x <= 1; x++)
	{
		for (int y = -1; y <= 1; y++)
		{
			ivec2 offset = ivec2(x, y);
			vec3 samp = texelFetch(u_IntermediateOutput, ivec2(texcoord) + offset, 0).rgb;
			minSample = min(samp, minSample);
			maxSample = max(samp, maxSample);

			avg += samp;
			avgWeight += 1.0f;
		}
	}
	avg = avg / avgWeight;

	// Read HistoryBuffer
	const vec2 jitter	= u_PerFrameBuffer.Val.Jitter;
	vec2 bestTexCoord	= FrontMostNeigbourTexCoord(texUV);
	vec2 velocity = texture(u_Velocity, bestTexCoord, 0).xy;
	velocity = (velocity * size);
	
	ivec2 prevTexcoord	= ivec2(texcoord + velocity);
	vec3 previousSample	= imageLoad(u_HistoryBuffer, prevTexcoord).rgb;
	previousSample		= ClipAABB(minSample, maxSample, previousSample, avg);

	// Calculate result with weights (Luminance filtering)
	float prevWeight	= 0.9f;
	float currentWeight	= 1.0f - prevWeight;
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