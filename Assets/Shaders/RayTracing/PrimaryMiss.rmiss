#version 460
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_ray_tracing : enable

#include "../Helpers.glsl"

struct SPrimaryPayload
{
	float Distance;
};

layout(location = 0) rayPayloadInEXT SPrimaryPayload s_PrimaryPayload;

void main() 
{
    s_PrimaryPayload.Distance = 0.0f;
}