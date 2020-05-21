#version 460
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_debug_printf : enable

#include "Helpers.glsl"

struct SRayPayload
{
    uint AllowPrints;
	vec3 Color;
};

layout(location = 0) rayPayloadInEXT SRayPayload s_RayPayload;

hitAttributeEXT vec3 attribs;

void main() 
{
    s_RayPayload.Color = vec3(0.0f, 1.0f, 0.0f);

    if (s_RayPayload.AllowPrints == 1)
    {
        debugPrintfEXT("Hit");
    }
}