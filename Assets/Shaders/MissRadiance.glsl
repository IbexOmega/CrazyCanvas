#version 460
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_ray_tracing : enable
//#extension GL_EXT_debug_printf : enable

#include "Helpers.glsl"

struct SRadiancePayload
{
	vec3    OutgoingRadiance;
};

layout(location = 0) rayPayloadInEXT SRadiancePayload s_RadiancePayload;

void main() 
{
    s_RadiancePayload.OutgoingRadiance  = vec3(0.529f, 0.808, 0.922f);
}