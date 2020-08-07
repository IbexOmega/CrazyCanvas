#version 460
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_ray_tracing : enable

#include "Helpers.glsl"
#include "RayTracingInclude.glsl"

layout(location = 1) rayPayloadInEXT SShadowPayload s_ShadowPayload;

void main() 
{
    s_ShadowPayload.Distance = gl_HitTEXT;
}