#version 460
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_buffer_reference : enable
#extension GL_EXT_scalar_block_layout : enable

#include "../Helpers.glsl"
#include "../Defines.glsl"

layout(binding = 0,     set = BUFFER_SET_INDEX) uniform PerFrameBuffer              { SPerFrameBuffer val; }    u_PerFrameBuffer;
layout(binding = 1,     set = BUFFER_SET_INDEX) uniform accelerationStructureEXT                                u_TLAS;

layout(binding = 0,    set = TEXTURE_SET_INDEX, rgba8) restrict uniform image2D u_OutputTexture;

layout(buffer_reference, buffer_reference_align = 16) buffer VertexBuffer 
{
    SVertex v[];
};

layout(buffer_reference, buffer_reference_align = 4, scalar) buffer IndexBuffer 
{
    uvec3 t[];
};

layout(shaderRecordEXT) buffer SBTData 
{
    VertexBuffer vertex;
    IndexBuffer indices;
};

struct SPrimaryPayload
{
	float Distance;
};

layout(location = 0) rayPayloadInEXT SPrimaryPayload s_PrimaryPayload;

void main() 
{
    const uvec3 tri = indices.t[gl_PrimitiveID];

    s_PrimaryPayload.Distance = float(tri.x) / 100.0f;
}