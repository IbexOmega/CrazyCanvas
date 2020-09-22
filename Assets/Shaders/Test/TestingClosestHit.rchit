#version 460
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_buffer_reference : enable
#extension GL_EXT_scalar_block_layout : enable

struct SPrimaryPayload
{
    float Hit;
};

layout(location = 0) rayPayloadInEXT SPrimaryPayload s_PrimaryPayload;

void main() 
{
	s_PrimaryPayload.Hit = 1.0f;
}