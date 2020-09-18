#version 460
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_ray_tracing : enable

#include "../Helpers.glsl"

layout(binding = 0,     set = BUFFER_SET_INDEX) uniform PerFrameBuffer              { SPerFrameBuffer val; }    u_PerFrameBuffer;
layout(binding = 1,     set = BUFFER_SET_INDEX) uniform accelerationStructureEXT                                u_TLAS;

layout(binding = 0,    set = TEXTURE_SET_INDEX, rgba8) restrict uniform image2D u_OutputTexture;

struct SPrimaryPayload
{
	float Distance;
};

layout(location = 0) rayPayloadInEXT SPrimaryPayload s_PrimaryPayload;

void main() 
{
    s_PrimaryPayload.Distance = 0.5f;
}