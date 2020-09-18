#version 460
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_buffer_reference : enable
#extension GL_EXT_scalar_block_layout : enable

#include "../Helpers.glsl"
#include "../Defines.glsl"

layout(buffer_reference, buffer_reference_align = 16) buffer VertexBuffer 
{
    SVertex v[];
};

layout(buffer_reference, buffer_reference_align = 4, scalar) buffer IndexBuffer 
{
    uint i[];
};

layout(shaderRecordEXT) buffer SBTData 
{
    vec4 color;
};

struct SPrimaryPayload
{
	float Distance;
};

layout(location = 0) rayPayloadInEXT SPrimaryPayload s_PrimaryPayload;

void main() 
{
    //const uint idx = indices.i[3 * gl_PrimitiveID];

    s_PrimaryPayload.Distance = color.x;//float(idx) / 100.0f;
}