#version 460
#extension GL_NV_ray_tracing : require

struct ShadowRayPayload 
{
	float Occlusion;
};

layout(location = 1) rayPayloadInNV ShadowRayPayload shadowRayPayload;

void main()
{
	shadowRayPayload.Occlusion = 0.0f;
}