#version 450 core
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

vec4 ChannelBitMult(uint mask, uint dstChannel)
{
    uint shift = (3 - dstChannel) * 4;
    return vec4(
        float((mask >> (shift + 3)) & 0x00000001), 
        float((mask >> (shift + 2)) & 0x00000001), 
        float((mask >> (shift + 1)) & 0x00000001), 
        float((mask >> (shift + 0)) & 0x00000001));
}   

void main()
{
    uint includeMask     = (u_PC.ReservedIncludeMask) & 0x0000FFFF;
    //uint reservedMask      = (u_PC.ReservedIncludeMask >> 16) & 0x0000FFFF;

    vec4 sampledColor = texture(sTexture, In.UV.st);

    const float zNear = 0.001f;
    const float zFar = 1000.0f;
    sampledColor.r = zNear * zFar / (zFar + sampledColor.r * (zNear - zFar));

    float R = dot(ChannelBitMult(includeMask, 0), sampledColor);
    float G = dot(ChannelBitMult(includeMask, 1), sampledColor);
    float B = dot(ChannelBitMult(includeMask, 2), sampledColor);
    float A = dot(ChannelBitMult(includeMask, 3), sampledColor);

    vec4 finalColor = vec4(R, G, B, A) * u_PC.Mult + u_PC.Add;

    fColor = In.Color * finalColor;
}