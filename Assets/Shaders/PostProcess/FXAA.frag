#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable

#include "../Defines.glsl"

layout(location = 0) in vec2 in_TexCoord;

layout(binding = 0, set = NO_BUFFERS_TEXTURE_SET_INDEX) uniform sampler2D u_BackBuffer;

layout(location = 0) out vec4 out_Color;

#define FXAA_EDGE_THRESHOLD			(1.0f / 8.0f)
#define FXAA_EDGE_THRESHOLD_MIN		(1.0f / 24.0f)

#define FXAA_SUBPIX_TRIM			(1.0f / 4.0f)
#define FXAA_SUBPIX_CAP				(3.0f / 4.0f)
#define FXAA_SUBPIX_TRIM_SCALE		(1.0f / (1.0f - FXAA_SUBPIX_TRIM))

#define FXAA_SEARCH_THRESHOLD		(1.0f / 4.0f)
#define FXAA_SEARCH_STEPS			24

vec3 Lerp(vec3 a, vec3 b, float amountOfA)
{
	vec3 t0 = vec3(-amountOfA) * b;
	vec3 t1 = (a * vec3(amountOfA)) + b;
	return t0 + t1;
}

void main()
{
	// Perform edge detection
	vec4 middle = textureOffset(u_BackBuffer, in_TexCoord, ivec2( 0,  0));
	vec4 north  = textureOffset(u_BackBuffer, in_TexCoord, ivec2( 0,  1));
	vec4 south  = textureOffset(u_BackBuffer, in_TexCoord, ivec2( 0, -1));
	vec4 west   = textureOffset(u_BackBuffer, in_TexCoord, ivec2(-1,  0));
	vec4 east   = textureOffset(u_BackBuffer, in_TexCoord, ivec2( 1,  0));
	float lumaM = middle.a;
	float lumaN = north.a;
	float lumaS = south.a;
	float lumaW = west.a;
	float lumaE = east.a;
	
//	out_Color = vec4(middle.rgb, 1.0f);
//	return;

	float rangeMin = min(lumaM, min(min(lumaN, lumaS), min(lumaW, lumaE)));
	float rangeMax = max(lumaM, max(max(lumaN, lumaS), max(lumaW, lumaE)));
	float range = rangeMax - rangeMin;
	if (range < max(FXAA_EDGE_THRESHOLD_MIN, rangeMax * FXAA_EDGE_THRESHOLD))
	{
		out_Color = vec4(middle.rgb, 1.0f);
		return;
	}

	vec2 texSize = textureSize(u_BackBuffer, 0);
	vec2 invTexSize = vec2(1.0f) / texSize;
	float lumaL		= (lumaN + lumaS + lumaW + lumaE) * 0.25f;
	float rangeL	= abs(lumaL - lumaM);
	float blendL	= max(0.0f, (rangeL / range) - FXAA_SUBPIX_TRIM) * FXAA_SUBPIX_TRIM_SCALE;
	blendL = min(blendL, FXAA_SUBPIX_CAP);
	
	vec4 northWest = textureOffset(u_BackBuffer, in_TexCoord, ivec2(-1,  1));
	vec4 southWest = textureOffset(u_BackBuffer, in_TexCoord, ivec2(-1, -1));
	vec4 northEast = textureOffset(u_BackBuffer, in_TexCoord, ivec2( 1,  1));
	vec4 southEast = textureOffset(u_BackBuffer, in_TexCoord, ivec2( 1, -1));
	
	vec3 colorL = (middle.rgb + north.rgb + south.rgb + west.rgb + east.rgb);
	colorL += (northWest.rgb + southWest.rgb + northEast.rgb + southEast.rgb);
	colorL = colorL * vec3(1.0f / 9.0f);

	float lumaNW = northWest.a;
	float lumaNE = northEast.a;
	float lumaSW = southWest.a;
	float lumaSE = southEast.a;
	
	float lumaNorthSouth	= lumaN + lumaS;
	float lumaWestEast		= lumaW + lumaE;
	float lumaWestCorners	= lumaSW + lumaNW;
	float lumaSouthCorners	= lumaSW + lumaSE;
	float lumaEastCorners	= lumaSE + lumaNE;
	float lumaNorthCorners	= lumaNW + lumaNE;
	
	float edgeVert =
		(abs((-2.0f * lumaN) + lumaNorthCorners))	+
		(abs((-2.0f * lumaM) + lumaWestEast) * 2.0f) +
		(abs((-2.0f * lumaS) + lumaSouthCorners));
	float edgeHorz =
		(abs((-2.0f * lumaW) + lumaWestCorners))	+
		(abs((-2.0f * lumaM) + lumaNorthSouth) * 2.0f)	+
		(abs((-2.0f * lumaE) + lumaEastCorners));
	
	bool isHorizontal = (edgeHorz >= edgeVert);
	if (!isHorizontal)
	{
		lumaN = lumaW;
	}
	if (!isHorizontal)
	{
		lumaS = lumaE;
	}
	
	float gradientN = abs(lumaN - lumaM);
	float gradientS = abs(lumaS - lumaM);
	float lumaAvgN = (lumaN + lumaM) * 0.5f;
	float lumaAvgS = (lumaS + lumaM) * 0.5f;
	
	bool pairN = (abs(gradientN) >= abs(gradientS));
	float localLumaAvg	= pairN ? lumaAvgN	: lumaAvgS;
	float localGradient = pairN ? gradientN	: gradientS;
	localGradient = localGradient * FXAA_SEARCH_THRESHOLD;

	float stepLength = (!isHorizontal) ? -invTexSize.y : invTexSize.x;
	if (pairN)
	{
		stepLength = -stepLength;
	}
	
	vec2 currentTexCoord = in_TexCoord;
	if (isHorizontal)
	{
		currentTexCoord.y += stepLength * 0.5f;
	}
	else
	{
		currentTexCoord.x += stepLength * 0.5f;
	}
	
	vec2 offset = isHorizontal ? vec2(invTexSize.x, 0.0f) : vec2(0.0f, invTexSize.y);
	bool done0 = false;
	bool done1 = false;
	float lumaEnd0;
	float lumaEnd1;
	vec2 texCoord0 = currentTexCoord - offset;
	vec2 texCoord1 = currentTexCoord + offset;

	
	int steps = 0;
	for (; steps < FXAA_SEARCH_STEPS; steps++)
	{
		if (!done0)
		{
			vec4 sample0 = texture(u_BackBuffer, texCoord0);
			lumaEnd0 = sample0.a;
		}
		if(!done1)
		{
			vec4 sample1 = texture(u_BackBuffer, texCoord1);
			lumaEnd1 = sample1.a;
		}
		
		done0 = (abs(lumaEnd0 - localLumaAvg) >= localGradient);
		done1 = (abs(lumaEnd1 - localLumaAvg) >= localGradient);
		if (done0 && done1)
		{
			break;
		}
		
		if (!done0)
		{
			texCoord0 -= offset;
		}
		if (!done1)
		{
			texCoord1 += offset;
		}
	}
	
	float distance0 = isHorizontal ? (in_TexCoord.x - texCoord0.x) : (in_TexCoord.y - texCoord0.y);
	float distance1 = isHorizontal ? (texCoord1.x - in_TexCoord.x) : (texCoord1.y - in_TexCoord.y);
	bool direction0	= distance0 < distance1;
	lumaEnd0 = direction0 ? lumaEnd0 : lumaEnd0;
	
	if (((lumaM - lumaN) < 0.0f) == ((lumaEnd0 - lumaN) < 0.0f))
	{
		stepLength = 0.0f;
	}
	
	float spanLength = (distance0 + distance1);
	distance0 = direction0 ? distance0 : distance1;
	
	float subPixelOffset = (0.5f + (distance0 * (1.0f / spanLength))) * stepLength;
	vec2 finalTexCoord = in_TexCoord + vec2(isHorizontal ? 0.0f : subPixelOffset, isHorizontal ? subPixelOffset : 0.0f);
	
	vec3 colorF = texture(u_BackBuffer, finalTexCoord).rgb;
	vec3 finalColor = Lerp(colorL, colorF, blendL);

	out_Color = vec4(finalColor, 1.0f);
}