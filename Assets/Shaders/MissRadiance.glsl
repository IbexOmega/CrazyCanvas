#version 460
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_ray_tracing : enable

#include "Helpers.glsl"
#include "RayTracingInclude.glsl"

layout(location = 0) rayPayloadInEXT SRadiancePayload s_RadiancePayload;

void main() 
{
    s_RadiancePayload.L  = vec3(0.529f, 0.808, 0.922f);
}