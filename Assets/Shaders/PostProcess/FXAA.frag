#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable

#include "../Defines.glsl"

layout(location = 0) in vec2 in_TexCoord;

layout(binding = 0, set = NO_BUFFERS_TEXTURE_SET_INDEX) uniform sampler2D u_BackBuffer;

layout(location = 0) out vec4 out_Color;

#define FXAA_EDGE_THRESHOLD		(1.0f / 8.0f)
#define FXAA_EDGE_THRESHOLD_MIN	(1.0f / 24.0f)
#define FXAA_SUBPIX_TRIM		(1.0f / 4.0f)
#define FXAA_SUBPIX_CAP			(3.0f / 4.0f)
#define FXAA_SUBPIX_TRIM_SCALE	(1.0f / (1.0f - FXAA_SUBPIX_TRIM))
#define FXAA_SEARCH_THRESHOLD	(1.0f / 4.0f)
#define FXAA_SEARCH_STEPS		16

vec3 Lerp(vec3 a, vec3 b, float amountOfA)
{
	vec3 t0 = vec3(-amountOfA) * b;
	vec3 t1 = (a * vec3(amountOfA)) + b;
	return t0 + t1;
}

#define PASSTHROUGH			0
#define DEBUG_EDGES			0
#define DEBUG				0
#define DEBUG_HORIZONTAL	0
#define DEBUG_NEGPOS		0
#define DEBUG_STEP			0
#define DEBUG_BLEND_FACTOR	0

void main()
{
	vec2 texCoord = in_TexCoord;
	vec4 m = textureLodOffset(u_BackBuffer, texCoord, 0.0f, ivec2( 0,  0));
#if PASSTHROUGH
	out_Color = vec4(m.rgb, 1.0f);
	return;
#endif

	vec4 n = textureLodOffset(u_BackBuffer, texCoord, 0.0f, ivec2( 0, -1));
	vec4 s = textureLodOffset(u_BackBuffer, texCoord, 0.0f, ivec2( 0,  1));
	vec4 w = textureLodOffset(u_BackBuffer, texCoord, 0.0f, ivec2(-1,  0));
	vec4 e = textureLodOffset(u_BackBuffer, texCoord, 0.0f, ivec2( 1,  0));
	float lumaM = m.a;
	float lumaN = n.a;
	float lumaS = s.a;
	float lumaW = w.a;
	float lumaE = e.a;

	// return if we determine that this is not an edge	
	float rangeMin = min(lumaM, min(min(lumaN, lumaS), min(lumaW, lumaE)));
	float rangeMax = max(lumaM, max(max(lumaN, lumaS), max(lumaW, lumaE)));
	float range = rangeMax - rangeMin;
	if (range < max(FXAA_EDGE_THRESHOLD_MIN, rangeMax * FXAA_EDGE_THRESHOLD))
	{
#if DEBUG
		out_Color = vec4(vec3(m.a), 1.0f);
#else
		out_Color = vec4(m.rgb, 1.0f);
#endif
		return;
	}

#if DEBUG_EDGES
	out_Color = vec4(1.0f);
	return;
#endif

	// Size
	const vec2 texSize		= textureSize(u_BackBuffer, 0);
	const vec2 invTexSize	= vec2(1.0f) / texSize;

	float lumaL		= (lumaN + lumaS + lumaW + lumaE) * 0.25f;
	float rangeL	= abs(lumaL - lumaM);
	float blendL	= max(0.0f, (rangeL / range) - FXAA_SUBPIX_TRIM) * FXAA_SUBPIX_TRIM_SCALE;
	blendL			= min(blendL, FXAA_SUBPIX_CAP);
	
#if DEBUG_BLEND_FACTOR
	out_Color = vec4(blendL);
	return;
#endif

	vec4 nw = textureLodOffset(u_BackBuffer, texCoord, 0.0f, ivec2(-1, -1));
	vec4 sw = textureLodOffset(u_BackBuffer, texCoord, 0.0f, ivec2(-1,  1));
	vec4 ne = textureLodOffset(u_BackBuffer, texCoord, 0.0f, ivec2( 1, -1));
	vec4 se = textureLodOffset(u_BackBuffer, texCoord, 0.0f, ivec2( 1,  1));
	
	vec3 colorL = (m.rgb + n.rgb + s.rgb + w.rgb + e.rgb);
	colorL += (nw.rgb + sw.rgb + ne.rgb + se.rgb);
	colorL = colorL * vec3(1.0f / 9.0f);

	float lumaNW = nw.a;
	float lumaNE = ne.a;
	float lumaSW = sw.a;
	float lumaSE = se.a;

	float edgeVert = 
		abs((0.25f * lumaNW) + (-0.5f * lumaN) + (0.25f * lumaNE)) +
		abs((0.50f * lumaW ) + (-1.0f * lumaM) + (0.50f * lumaE )) +
		abs((0.25f * lumaSW) + (-0.5f * lumaS) + (0.25f * lumaSE));
	float edgeHorz = 
		abs((0.25f * lumaNW) + (-0.5f * lumaW) + (0.25f * lumaSW)) +
		abs((0.50f * lumaN ) + (-1.0f * lumaM) + (0.50f * lumaS )) +
		abs((0.25f * lumaNE) + (-0.5f * lumaE) + (0.25f * lumaSE));
	
	bool isHorizontal = (edgeHorz >= edgeVert);
#if DEBUG_HORIZONTAL
	if(isHorizontal)
	{
		out_Color = vec4(0.0f, 1.0f, 0.0f, 1.0f);
		return;
	}
	else
	{
		out_Color = vec4(0.0f, 0.0f, 1.0f, 1.0f);
		return;
	}
#endif

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
	
	bool	pairN			= (gradientN >= gradientS);
	float	localLumaAvg	= (!pairN) ? lumaAvgS	: lumaAvgN;
	float	localGradient	= (!pairN) ? gradientS	: gradientN;
	localGradient			= localGradient * FXAA_SEARCH_THRESHOLD;

	float stepLength = isHorizontal ? -invTexSize.y : -invTexSize.x;
	if (!pairN)
	{
		stepLength *= -1.0f;
	}
	
	vec2 currentTexCoord = texCoord;
	if (isHorizontal)
	{
		currentTexCoord.y += stepLength * 0.5f;
	}
	else
	{
		currentTexCoord.x += stepLength * 0.5f;
	}
	
	vec2	offset		= isHorizontal ? vec2(invTexSize.x, 0.0f) : vec2(0.0f, invTexSize.y);
	bool	done0		= false;
	bool	done1		= false;
	float	lumaEnd0	= localLumaAvg;
	float	lumaEnd1	= localLumaAvg;
	vec2	texCoord0	= currentTexCoord - offset;
	vec2	texCoord1	= currentTexCoord + offset;

	for (int steps = 0; steps < FXAA_SEARCH_STEPS; steps++)
	{
		if (!done0)
		{
			vec4 sample0 = textureLod(u_BackBuffer, texCoord0, 0);
			lumaEnd0 = sample0.a;
		}
		if(!done1)
		{
			vec4 sample1 = textureLod(u_BackBuffer, texCoord1, 0);
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
	
	float distance0 = isHorizontal ? (texCoord.x - texCoord0.x) : (texCoord.y - texCoord0.y);
	float distance1 = isHorizontal ? (texCoord1.x - texCoord.x) : (texCoord1.y - texCoord.y);
	
	bool dir0 = distance0 < distance1;
#if DEBUG_NEGPOS
	if(dir0)
	{
		out_Color = vec4(1.0f, 0.0f, 0.0f, 1.0f);
		return;
	}
	else
	{
		out_Color = vec4(0.0f, 0.0f, 1.0f, 1.0f);
		return;
	}
#endif

	lumaEnd0 = dir0 ? lumaEnd0 : lumaEnd1;
	if (((lumaM - localLumaAvg) < 0.0f) == ((lumaEnd0 - localLumaAvg) < 0.0f))
	{
		stepLength = 0.0f;
	}

#if DEBUG_STEP
	if (stepLength == 0.0f)
	{
		out_Color = vec4(1.0f);
		return;
	}
#endif
	
	float spanLength	= (distance0 + distance1);
	distance0			= dir0 ? distance0 : distance1;
	
	float	subPixelOffset	= (0.5f + (distance0 * (-1.0f / spanLength))) * stepLength;
	vec2	finalTexCoord	= vec2(texCoord.x + (isHorizontal ? 0.0f : subPixelOffset), texCoord.y + (isHorizontal ? subPixelOffset : 0.0f));
	vec3	colorF			= texture(u_BackBuffer, finalTexCoord).rgb;
	vec3	finalColor		= Lerp(colorL, colorF, blendL);
	out_Color = vec4(finalColor, 1.0f);
}