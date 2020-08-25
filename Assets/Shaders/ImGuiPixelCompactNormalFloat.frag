#version 450 core
#extension GL_GOOGLE_include_directive : enable

#include "ImGuiHelpers.glsl"
#include "Helpers.glsl"

layout(location = 0) in struct { vec4 Color; vec2 UV; } In;

layout(set=0, binding=0) uniform sampler2D sTexture;

layout(push_constant) uniform PushConstants 
{ 
    layout(offset = 16)
    vec4 Mult; 
    vec4 Add; 
    uint ReservedIncludeMask;
} u_PC;

layout(location = 0) out vec4 fColor;

void main()
{
    uint includeMask     = (u_PC.ReservedIncludeMask) & 0x0000FFFF;
    //uint reservedMask      = (u_PC.ReservedIncludeMask >> 16) & 0x0000FFFF;

    vec4 sampledColor   = texture(sTexture, In.UV.st);
    vec4 mult           = vec4(ChannelBitMult(includeMask, 0));
    
    vec3 normal = vec3(0.0f);
    if (mult.x > 0.0f)  normal  = octToDir(floatBitsToUint(sampledColor.x));
    if (mult.y > 0.0f)  normal  = octToDir(floatBitsToUint(sampledColor.y));
    if (mult.z > 0.0f)  normal  = octToDir(floatBitsToUint(sampledColor.z));
    if (mult.w > 0.0f)  normal  = octToDir(floatBitsToUint(sampledColor.w));

    vec4 finalColor   = vec4(normal, 0.0f) * u_PC.Mult + u_PC.Add;

    fColor = In.Color * finalColor;
}