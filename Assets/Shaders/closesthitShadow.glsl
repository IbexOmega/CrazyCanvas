#version 460
#extension GL_NV_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable

struct ShadowRayPayload 
{
	float Occlusion;
};

layout(location = 1) rayPayloadInNV ShadowRayPayload shadowRayPayload;

void main()
{
	shadowRayPayload.Occlusion = 1.0f;
}
