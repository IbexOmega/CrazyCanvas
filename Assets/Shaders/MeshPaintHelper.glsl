#ifndef MESHPAINTHELPER_SHADER
#define MESHPAINTHELPER_SHADER

#define PAINT_NOISE_SCALE		8.0f
#define PAINT_DELTA_NOISE		0.0005f
#define PAINT_BUMPINESS	        10.0f
#define PAINT_ROUGHNESS	        0.2f

#include "Noise.glsl"

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

SPaintSample SamplePaint(in ivec2 p, in uint paintInfo, in float strength)
{
	uint client = (paintInfo >> 4) & 0x0F;
	uint server = paintInfo & 0x0F;

	//uvec2 paintData = floatBitsToUint(texelFetch(u_PaintMaskTextures[paintMaskIndex], p, 0).rg);

	uint clientTeam				= client & 0x0F;
	uint serverTeam				= server & 0x0F;
	uint clientPainting			= uint(step(1, int(client)));

	SPaintSample paintSample;
	paintSample.PaintAmount		= float(uint(step(1, int(client))) | uint(step(1, int(server)))) * (1.f - strength);
	paintSample.Team 			= clientPainting * clientTeam + (1 - clientPainting) * serverTeam;
	return paintSample;
}


SPaintDescription InterpolatePaint(in mat3 TBN, in vec3 position, in vec3 tangent, in vec3 bitangent, in vec2 texCoord, in uint paintInfo, in float strength)
{
	ivec2 paintMaskSize 		= ivec2(64);//textureSize(u_PaintMaskTextures[paintMaskIndex], 0);
	vec2 texelPos				= (texCoord * vec2(paintMaskSize));
	vec2 texelCenter;
	vec2 subTexel				= modf(texelPos, texelCenter);
	ivec2 iTexelCenter			= ivec2(texelCenter);

	SPaintSample paintSample00		= SamplePaint(iTexelCenter + ivec2(0, 0), 	paintInfo, strength);
	SPaintSample paintSample10		= SamplePaint(iTexelCenter + ivec2(1, 0), 	paintInfo, strength);
	SPaintSample paintSample01		= SamplePaint(iTexelCenter + ivec2(0, 1), 	paintInfo, strength);
	SPaintSample paintSample11		= SamplePaint(iTexelCenter + ivec2(1, 1), 	paintInfo, strength);

	vec3 paintColor00		= b_PaintMaskColor.val[paintSample00.Team].rgb;
	vec3 paintColor10 		= b_PaintMaskColor.val[paintSample10.Team].rgb;
	vec3 paintColor01 		= b_PaintMaskColor.val[paintSample01.Team].rgb;
	vec3 paintColor11 		= b_PaintMaskColor.val[paintSample11.Team].rgb;
	
	vec3 paintColorHor0 	= mix(paintColor00, paintColor10, subTexel.x);
	vec3 paintColorHor1 	= mix(paintColor01, paintColor11, subTexel.x);
	vec3 paintColorFinal	= mix(paintColorHor0, paintColorHor1, subTexel.y);

	float paintAmountHor0	= mix(paintSample00.PaintAmount, paintSample10.PaintAmount, subTexel.x);
	float paintAmountHor1	= mix(paintSample01.PaintAmount, paintSample11.PaintAmount, subTexel.x);
	float paintAmountFinal	= mix(paintAmountHor0, paintAmountHor1, subTexel.y);

	float h0 					= snoise(PAINT_NOISE_SCALE * (position));
	float h_u 					= snoise(PAINT_NOISE_SCALE * (position + PAINT_DELTA_NOISE * tangent));
	float h_v 					= snoise(PAINT_NOISE_SCALE * (position + PAINT_DELTA_NOISE * tangent));
	vec2 grad_h					= vec2(h0 - h_u, h0 - h_v);
	vec3 paintNormal			= normalize(vec3(PAINT_BUMPINESS * grad_h, sqrt(1.0f - (grad_h.x * grad_h.x) - (grad_h.y * grad_h.y))));
	vec3 noPaintNormal00		= normalize(vec3(-1.0f,  1.0f, 1.0f));
	vec3 noPaintNormal10		= normalize(vec3( 1.0f,  1.0f, 1.0f));
	vec3 noPaintNormal01		= normalize(vec3(-1.0f, -1.0f, 1.0f));
	vec3 noPaintNormal11		= normalize(vec3( 1.0f, -1.0f, 1.0f));
	vec3 combinedNormal			= TBN * normalize(
		paintNormal + 
		(1.0f - paintSample00.PaintAmount) * noPaintNormal00 +
		(1.0f - paintSample10.PaintAmount) * noPaintNormal10 +
		(1.0f - paintSample01.PaintAmount) * noPaintNormal01 +
		(1.0f - paintSample11.PaintAmount) * noPaintNormal11
	);

	SPaintDescription paintDescription;

	paintDescription.Normal			= combinedNormal;
	paintDescription.Albedo 		= vec3(strength);//paintColorFinal;
	paintDescription.Roughness 		= PAINT_ROUGHNESS;
	paintDescription.Interpolation	= 1.f;//paintAmountFinal;

	return paintDescription;
}

#endif
