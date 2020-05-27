#version 460
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_ray_tracing : enable
//#extension GL_EXT_debug_printf : enable

#include "Helpers.glsl"
#include "Defines.glsl"

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

layout(binding = 0, set = BUFFER_SET_INDEX) uniform accelerationStructureEXT   u_TLAS;
layout(binding = 1, set = BUFFER_SET_INDEX) uniform PerFrameBuffer     { SPerFrameBuffer val; }        u_PerFrameBuffer;

layout(binding = 0, set = TEXTURE_SET_INDEX) uniform sampler2D 	                u_AlbedoAO;
layout(binding = 1, set = TEXTURE_SET_INDEX) uniform sampler2D 	                u_NormalMetallicRoughness;
layout(binding = 2, set = TEXTURE_SET_INDEX) uniform sampler2D 	                u_DepthStencil;
layout(binding = 3, set = TEXTURE_SET_INDEX, rgba8) writeonly uniform image2D   u_Radiance;

layout(location = 0) rayPayloadEXT SRayPayload s_RayPayload;

void main()
{
    //Calculate Screen Coords
	const ivec2 pixelCoords = ivec2(gl_LaunchIDEXT.xy);
	const vec2 pixelCenter = vec2(pixelCoords) + vec2(0.5f);
	vec2 screenTexCoord = (pixelCenter / vec2(gl_LaunchSizeEXT.xy));
	vec2 d = screenTexCoord * 2.0 - 1.0;

	//Sample GBuffer
	//vec4 sampledNormalMetallicRoughness = texture(u_NormalMetallicRoughness, screenTexCoord);
	//vec3 normal = CalculateNormal(sampledNormalMetallicRoughness);

    //Skybox
	// if (dot(sampledNormalMetallicRoughness, sampledNormalMetallicRoughness) < EPSILON)
	// {
	// 	imageStore(u_Radiance, pixelCoords, vec4(1.0f, 0.0f, 1.0f, 1.0f));
	// 	return;
	// }

    SPerFrameBuffer perFrameBuffer              = u_PerFrameBuffer.val;
	
	

	//Sample GBuffer
	//vec4 sampledAlbedoAO    = texture(u_AlbedoAO, screenTexCoord);
	//float sampledDepth      = texture(u_DepthStencil, screenTexCoord).r;

	//Define Constants
	//SPositions positions            = CalculatePositionsFromDepth(screenTexCoord, sampledDepth, perFrameBuffer.ProjectionInv, perFrameBuffer.ViewInv);
    //SRayDirections rayDirections    = CalculateRayDirections(positions.WorldPos, normal, perFrameBuffer.Position.xyz, perFrameBuffer.ViewInv);

	vec4 origin 	= perFrameBuffer.ViewInv 		* vec4(0.0f , 0.0f, 0.0f, 1.0f);
	vec4 target 	= perFrameBuffer.ProjectionInv 	* vec4(d.x, d.y, 1.0f, 1.0f);
	vec4 direction 	= perFrameBuffer.ViewInv 		* vec4(normalize(target.xyz), 0.0f);

	//Define new Rays Parameters
	const uint 		rayFlags           	= gl_RayFlagsOpaqueEXT/* | gl_RayFlagsTerminateOnFirstHitEXT*/;
	const uint 		cullMask           	= 0xFF;
    const uint 		sbtRecordOffset    	= 0;
    const uint 		sbtRecordStride    	= 0;
    const uint 		missIndex          	= 0;
    const float 	Tmin              	= 0.001f;
	const float 	Tmax              	= 10000.0f;
    const int 		payload       		= 0;

	if (pixelCoords == ivec2(0))
	{
		//debugPrintfEXT("Snopp");
		//debugPrintfEXT("Pos: %f, %f, %f Dir: %f, %f, %f", origin.x, origin.y, origin.z, direction.x, direction.y, direction.z);
		s_RayPayload.AllowPrints = 1;
	}
	else
	{
		s_RayPayload.AllowPrints = 0;
	}

	s_RayPayload.Color = vec3(1.0f, 0.0f, 0.0f);
    traceRayEXT(u_TLAS, rayFlags, cullMask, sbtRecordOffset, sbtRecordStride, missIndex, origin.xyz, Tmin, direction.xyz, Tmax, payload);

	imageStore(u_Radiance, pixelCoords, vec4(s_RayPayload.Color, 1.0f));

	// if (screenTexCoord.x < 0.25f)
	// {
	// 	if (screenTexCoord.y < 0.25f)
	// 	{
	// 		imageStore(u_Radiance, pixelCoords, vec4(vec3(perFrameBuffer.ProjectionInv[0].x), 1.0f));
	// 	}
	// 	else if (screenTexCoord.y < 0.5f)
	// 	{
	// 		imageStore(u_Radiance, pixelCoords, vec4(vec3(perFrameBuffer.ProjectionInv[0].y), 1.0f));
	// 	}
	// 	else if (screenTexCoord.y < 0.75f)
	// 	{
	// 		imageStore(u_Radiance, pixelCoords, vec4(vec3(perFrameBuffer.ProjectionInv[0].z), 1.0f));
	// 	}
	// 	else
	// 	{
	// 		imageStore(u_Radiance, pixelCoords, vec4(vec3(perFrameBuffer.ProjectionInv[0].w), 1.0f));
	// 	}
	// }
	// else if (screenTexCoord.x < 0.5f)
	// {
	// 	if (screenTexCoord.y < 0.25f)
	// 	{
	// 		imageStore(u_Radiance, pixelCoords, vec4(vec3(perFrameBuffer.ProjectionInv[1].x), 1.0f));
	// 	}
	// 	else if (screenTexCoord.y < 0.5f)
	// 	{
	// 		imageStore(u_Radiance, pixelCoords, vec4(vec3(perFrameBuffer.ProjectionInv[1].y), 1.0f));
	// 	}
	// 	else if (screenTexCoord.y < 0.75f)
	// 	{
	// 		imageStore(u_Radiance, pixelCoords, vec4(vec3(perFrameBuffer.ProjectionInv[1].z), 1.0f));
	// 	}
	// 	else
	// 	{
	// 		imageStore(u_Radiance, pixelCoords, vec4(vec3(perFrameBuffer.ProjectionInv[1].w), 1.0f));
	// 	}
	// }
	// else if (screenTexCoord.x < 0.75f)
	// {
	// 	if (screenTexCoord.y < 0.25f)
	// 	{
	// 		imageStore(u_Radiance, pixelCoords, vec4(vec3(perFrameBuffer.ProjectionInv[2].x), 1.0f));
	// 	}
	// 	else if (screenTexCoord.y < 0.5f)
	// 	{
	// 		imageStore(u_Radiance, pixelCoords, vec4(vec3(perFrameBuffer.ProjectionInv[2].y), 1.0f));
	// 	}
	// 	else if (screenTexCoord.y < 0.75f)
	// 	{
	// 		imageStore(u_Radiance, pixelCoords, vec4(vec3(perFrameBuffer.ProjectionInv[2].z), 1.0f));
	// 	}
	// 	else
	// 	{
	// 		imageStore(u_Radiance, pixelCoords, vec4(vec3(perFrameBuffer.ProjectionInv[2].w), 1.0f));
	// 	}
	// }
	// else
	// {
	// 	if (screenTexCoord.y < 0.25f)
	// 	{
	// 		imageStore(u_Radiance, pixelCoords, vec4(vec3(perFrameBuffer.ProjectionInv[3].x), 1.0f));
	// 	}
	// 	else if (screenTexCoord.y < 0.5f)
	// 	{
	// 		imageStore(u_Radiance, pixelCoords, vec4(vec3(perFrameBuffer.ProjectionInv[3].y), 1.0f));
	// 	}
	// 	else if (screenTexCoord.y < 0.75f)
	// 	{
	// 		imageStore(u_Radiance, pixelCoords, vec4(vec3(perFrameBuffer.ProjectionInv[3].z), 1.0f));
	// 	}
	// 	else
	// 	{
	// 		imageStore(u_Radiance, pixelCoords, vec4(vec3(perFrameBuffer.ProjectionInv[3].w), 1.0f));
	// 	}
	// }
}