#version 460
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_ray_tracing : enable

#include "helpers.glsl"

struct SRayPayload
{
	uint AllowPrints;
	vec3 Color;
};

struct SPerFrameBuffer
{
    mat4 Projection;
	mat4 View;
	mat4 LastProjection;
	mat4 LastView;
	mat4 ViewInv;
	mat4 ProjectionInv;
	vec4 Position;
	vec4 Right;
	vec4 Up;
};

layout(binding = 0, set = 0) uniform sampler2D 	                u_AlbedoAO;
layout(binding = 1, set = 0) uniform sampler2D 	                u_NormalMetallicRoughness;
layout(binding = 2, set = 0) uniform sampler2D 	                u_DepthStencil;
layout(binding = 3, set = 0, rgba8) writeonly uniform image2D   u_Radiance;

layout(binding = 0, set = 1) uniform accelerationStructureEXT   u_TLAS;
layout(binding = 1, set = 1) uniform PerFrameBuffer     { SPerFrameBuffer val; }        u_PerFrameBuffer;

layout(location = 0) rayPayloadEXT SRayPayload s_RayPayload;

void main()
{
    //Calculate Screen Coords
	const ivec2 pixelCoords = ivec2(gl_LaunchIDEXT.xy);
	const vec2 pixelCenter = vec2(pixelCoords) + vec2(0.5f);
	vec2 screenTexCoord = (pixelCenter / vec2(gl_LaunchSizeEXT.xy));

	//Sample GBuffer
	vec4 sampledNormalMetallicRoughness = texture(u_NormalMetallicRoughness, screenTexCoord);
	vec3 normal = CalculateNormal(sampledNormalMetallicRoughness);

    //Skybox
	// if (dot(sampledNormalMetallicRoughness, sampledNormalMetallicRoughness) < EPSILON)
	// {
	// 	imageStore(u_Radiance, pixelCoords, vec4(1.0f, 0.0f, 1.0f, 1.0f));
	// 	return;
	// }

    SPerFrameBuffer perFrameBuffer              = u_PerFrameBuffer.val;

	//Sample GBuffer
	vec4 sampledAlbedoAO    = texture(u_AlbedoAO, screenTexCoord);
	float sampledDepth      = texture(u_DepthStencil, screenTexCoord).r;

	//Define Constants
	SPositions positions            = CalculatePositionsFromDepth(screenTexCoord, sampledDepth, perFrameBuffer.ProjectionInv, perFrameBuffer.ViewInv);
    SRayDirections rayDirections    =  CalculateRayDirections(positions.WorldPos, normal, perFrameBuffer.Position.xyz, perFrameBuffer.ViewInv);

	vec2 d = screenTexCoord * 2.0 - 1.0;

	vec4 origin = perFrameBuffer.ViewInv * vec4(0.0f, 0.0f, 0.0f, 1.0f);
	vec4 target = perFrameBuffer.ProjectionInv * vec4(d.x, d.y, 1.0f, 1.0f);
	vec4 direction = perFrameBuffer.ViewInv * vec4(normalize(target.xyz / target.w), 0.0f) ;

	//Define new Rays Parameters
	uint rayFlags           = gl_RayFlagsOpaqueEXT | gl_RayFlagsTerminateOnFirstHitEXT;
	uint cullMask           = 0xFF;
    uint sbtRecordOffset    = 0;
    uint sbtRecordStride    = 0;
    uint missIndex          = 0;
    float Tmin              = 0.001f;
	float Tmax              = 10000.0f;
    const int payload       = 0;

	s_RayPayload.AllowPrints = pixelCoords == ivec2(0) ? 1 : 0;
	s_RayPayload.Color = vec3(0.0f, 0.0f, 1.0f);
    traceRayEXT(u_TLAS, rayFlags, cullMask, sbtRecordOffset, sbtRecordStride, missIndex, origin.xyz, Tmin, direction.xyz, Tmax, payload);

	imageStore(u_Radiance, pixelCoords, vec4(s_RayPayload.Color, 1.0f));
}