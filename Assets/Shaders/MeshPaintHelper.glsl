#ifndef MESHPAINTHELPER_SHADER
#define MESHPAINTHELPER_SHADER

#define PAINT_NOISE_SCALE			2.0f
#define PAINT_DELTA_NOISE			0.0003f
#define PAINT_BUMPINESS	        	50.0f
#define PAINT_BORDER_DELTA_NOISE	0.0005f
#define PAINT_BORDER_BUMPINESS	    15.0f
#define PAINT_BORDER_FREQUENCY	    10.0f
#define PAINT_BORDER_AMPLITUDE	    0.4f
#define PAINT_ROUGHNESS	        	0.1f

#include "Noise.glsl"
#include "MeshPaintFunc.glsl"

struct SPaintDescription
{
	vec3 	Normal;
	vec3 	Albedo;
	float 	Roughness;
	float 	Interpolation;
};

struct SPaintSample
{
	float PaintAmount;
	uint Team;
};

float GetPaintNoise(float freq, float amp, vec3 position, float dist)
{
	float paintNoise = snoise(position * freq) * amp + dist;
	return paintNoise;
}

float GetPaintBorder(float paintNoise)
{
	float b = PAINT_THRESHOLD;
	float lo = 1.f-smoothstep(b, b+0.2f, paintNoise);
	float hi = smoothstep(b-0.09f, b, paintNoise);
	b = lo*hi;
	return b;
}

SPaintSample SamplePaint(in vec3 position, in uint packedPaintInfo, in float paint)
{
	SPaintSample paintSample;
	paintSample.PaintAmount		= float(step(1, packedPaintInfo))*paint;
	paintSample.Team 			= packedPaintInfo;
	return paintSample;
}

SPaintDescription InterpolatePaint(in mat3 TBN, in vec3 position, in vec3 tangent, in vec3 bitangent, in uint packedPaintInfo, in float dist)
{
	float f = PAINT_FREQ;
	float a = PAINT_AMPL;
	float paintNoise = GetPaintNoise(f, a, position, dist);
	float paint = 1.f - smoothstep(PAINT_THRESHOLD, 0.5f, paintNoise);
	float b = GetPaintBorder(paintNoise);

	float paint00 = 1.f - smoothstep(PAINT_THRESHOLD, 0.5f, GetPaintNoise(f, a, position + tangent*PAINT_BORDER_DELTA_NOISE, dist));
	float paint01 = 1.f - smoothstep(PAINT_THRESHOLD, 0.5f, GetPaintNoise(f, a, position + bitangent*PAINT_BORDER_DELTA_NOISE, dist));
	float paint10 = 1.f - smoothstep(PAINT_THRESHOLD, 0.5f, GetPaintNoise(f, a, position - tangent*PAINT_BORDER_DELTA_NOISE, dist));
	float paint11 = 1.f - smoothstep(PAINT_THRESHOLD, 0.5f, GetPaintNoise(f, a, position - bitangent*PAINT_BORDER_DELTA_NOISE, dist));
	vec2 borderDir = vec2(paint01 - paint11, paint00 - paint10);

	SPaintSample paintSample = SamplePaint(position, packedPaintInfo, clamp(paint + b, 0.f, 1.f));
	vec3 paintColorFinal	= b_PaintMaskColor.val[paintSample.Team].rgb;
	float paintAmountFinal	= paintSample.PaintAmount;

	float h0 					= snoise(PAINT_NOISE_SCALE * (position));
	float h_u 					= snoise(PAINT_NOISE_SCALE * (position + PAINT_DELTA_NOISE * tangent));
	float h_v 					= snoise(PAINT_NOISE_SCALE * (position + PAINT_DELTA_NOISE * bitangent));
	vec2 grad_h					= vec2(h0 - h_u, h0 - h_v);
	vec3 paintNormal			= normalize(vec3(PAINT_BUMPINESS * grad_h, sqrt(1.0f - (grad_h.x * grad_h.x) - (grad_h.y * grad_h.y))));
	vec3 paintNormalBorder		= normalize(vec3(PAINT_BORDER_BUMPINESS*borderDir, sqrt(1.f - (borderDir.x*borderDir.x) - (borderDir.y*borderDir.y))));
	vec3 noPaintNormal00		= normalize(vec3(-1.0f,  1.0f, 1.0f));
	vec3 noPaintNormal10		= normalize(vec3( 1.0f,  1.0f, 1.0f));
	vec3 noPaintNormal01		= normalize(vec3(-1.0f, -1.0f, 1.0f));
	vec3 noPaintNormal11		= normalize(vec3( 1.0f, -1.0f, 1.0f));
	vec3 combinedNormal			= TBN * normalize(
		mix(paintNormal +
		(1.0f - paintAmountFinal) * noPaintNormal00 +
		(1.0f - paintAmountFinal) * noPaintNormal10 +
		(1.0f - paintAmountFinal) * noPaintNormal01 +
		(1.0f - paintAmountFinal) * noPaintNormal11, paintNormalBorder, b)
	);

	SPaintDescription paintDescription;

	paintDescription.Normal			= combinedNormal;
	paintDescription.Albedo 		= vec3(1.f - dist); //paintColorFinal;
	paintDescription.Roughness 		= PAINT_ROUGHNESS;
	paintDescription.Interpolation	= 1.f; //step(1.f, paintAmountFinal);

	return paintDescription;
}

#endif
