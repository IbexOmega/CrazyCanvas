#ifndef MESH_PAINT_FUNC_SHADER
#define MESH_PAINT_FUNC_SHADER

#define PAINT_THRESHOLD			0.4f

#include "Noise.glsl"

uint PackPaintInfo(in uint paintInfo)
{
	uint client = (paintInfo >> 4) & 0x0F;
	uint server = paintInfo & 0x0F;

	uint clientTeam				= client & 0x0F;
	uint serverTeam				= server & 0x0F;
	uint clientPainting			= uint(step(1, int(client)));
	uint serverPainting			= uint(step(1, int(server)));

	uint shouldPaint = clientPainting | serverPainting;
	uint teamIndex = clientPainting * clientTeam + (1 - clientPainting) * serverTeam;
	return shouldPaint*teamIndex;
}

vec4 PackedPaintInfoToVec4(in uint paintInfo)
{
    // This only supports four teams, because a vec4 has only four channels.
    // Will be 0 if no paint, 1 if it is paint.
    float t1 = 1. - float(step(1, abs(int(paintInfo) - 1)));
    float t2 = 1. - float(step(1, abs(int(paintInfo) - 2)));
    float t3 = 1. - float(step(1, abs(int(paintInfo) - 3)));
    float t4 = 1. - float(step(1, abs(int(paintInfo) - 4)));
    return vec4(t1, t2, t3, t4);
}

uint Vec4ToPackedPaintInfo(in vec4 v)
{
    // This only supports four teams, because a vec4 has only four channels.
    float t1 = v.x;
    float t2 = v.y;
    float t3 = v.z;
    float t4 = v.w;

    if(t1 > 0.001f && t1 > t2 && t1 > t3 && t1 > t4)
        return 1;
    if(t2 > 0.001f && t2 > t1 && t2 > t3 && t2 > t4)
        return 2;
    if(t3 > 0.001f && t3 > t1 && t3 > t2 && t3 > t4)
        return 3;
    if(t4 > 0.001f && t4 > t1 && t4 > t2 && t4 > t3)
        return 4;
    return 0;
}

void GetVec4ToPackedPaintInfoAndDistance(in vec3 pos, in vec4 v, in float dist, out uint packedPaintInfo, out float outDist)
{
    float isFullPaint = max(v.x, max(v.y, max(v.z, v.w)));
    float n1 = snoise(pos * 10.0f) * 0.4f;
    vec4 noiseV4 = v * (1.f-dist);
    float a = noiseV4.x + noiseV4.y + noiseV4.z + noiseV4.w;
    float b = step(0.5f, a);
    float c = (1.f-dist);
    vec4 vec4PaintInfo = clamp(vec4(v.x + n1*v.x*c, v.y - n1*v.y*c, v.z, v.w), 0.f, 1.f);
    packedPaintInfo = Vec4ToPackedPaintInfo(vec4PaintInfo);
    outDist = dist;
}

#endif
