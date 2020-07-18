#version 460
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_ray_tracing : enable
//#extension GL_EXT_debug_printf : enable

#include "Helpers.glsl"
#include "Defines.glsl"

struct SRayPayload
{
	vec3 IncomingRadiance;
};

layout(binding = 0, set = BUFFER_SET_INDEX) uniform accelerationStructureEXT   u_TLAS;
layout(binding = 6, set = BUFFER_SET_INDEX) uniform LightsBuffer       { SLightsBuffer val; }          u_LightsBuffer;
layout(binding = 7, set = BUFFER_SET_INDEX) uniform PerFrameBuffer     { SPerFrameBuffer val; }        u_PerFrameBuffer;

layout(binding = 0, set = TEXTURE_SET_INDEX) uniform sampler2D 	                u_AlbedoAO;
layout(binding = 1, set = TEXTURE_SET_INDEX) uniform sampler2D 	                u_NormalMetallicRoughness;
layout(binding = 2, set = TEXTURE_SET_INDEX) uniform sampler2D 	                u_DepthStencil;

layout(binding = 8, set = TEXTURE_SET_INDEX, rgba8) writeonly uniform image2D   u_Radiance;

layout(location = 0) rayPayloadEXT SRayPayload s_RayPayload;

void main()
{
    //Calculate Screen Coords
	const ivec2 pixelCoords = ivec2(gl_LaunchIDEXT.xy);
	const vec2 pixelCenter = vec2(pixelCoords) + vec2(0.5f);
	vec2 screenTexCoord = (pixelCenter / vec2(gl_LaunchSizeEXT.xy));
	vec2 d = screenTexCoord * 2.0 - 1.0;

	//Sample GBuffer
	vec4 sampledNormalMetallicRoughness = texture(u_NormalMetallicRoughness, screenTexCoord);

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

	//Unpack GBuffer
	vec3 albedo         = sampledAlbedoAO.rgb;
    vec3 normal         = CalculateNormal(sampledNormalMetallicRoughness);
    float ao            = sampledAlbedoAO.a;
    float metallic      = sampledNormalMetallicRoughness.b * 0.5f + 0.5f;
    float roughness     = abs(sampledNormalMetallicRoughness.a);

	//Define Constants
	SPositions positions            = CalculatePositionsFromDepth(screenTexCoord, sampledDepth, perFrameBuffer.ProjectionInv, perFrameBuffer.ViewInv);
    SRayDirections rayDirections    = CalculateRayDirections(positions.WorldPos, normal, perFrameBuffer.Position.xyz, perFrameBuffer.ViewInv);	

	//Define Reflection Ray Parameters
	const vec3 		origin 				= positions.WorldPos + normal * 0.025f;
	const vec3 		direction			= rayDirections.ReflDir;
	const uint 		rayFlags           	= gl_RayFlagsOpaqueEXT/* | gl_RayFlagsTerminateOnFirstHitEXT*/;
	const uint 		cullMask           	= 0xFF;
    const uint 		sbtRecordOffset    	= 0;
    const uint 		sbtRecordStride    	= 0;
    const uint 		missIndex          	= 0;
    const float 	Tmin              	= 0.001f;
	const float 	Tmax              	= 10000.0f;
    const int 		payload       		= 0;
	
	//Send Reflection Ray
	s_RayPayload.IncomingRadiance = vec3(0.0f, 0.0f, 0.0f);
    traceRayEXT(u_TLAS, rayFlags, cullMask, sbtRecordOffset, sbtRecordStride, missIndex, origin.xyz, Tmin, direction.xyz, Tmax, payload);

	//Calculate how much of the incoming radiance (from the Reflection Ray) gets reflected to the camera (Fresnel equations)
	vec3 F_0 = vec3(0.04f);
    F_0 = mix(F_0, s_RayPayload.IncomingRadiance, metallic);
	float cosTheta = max(dot(normal, rayDirections.ViewDir), 0.0f);

	vec3 outgoingRadiance = Fresnel(F_0, cosTheta);

	imageStore(u_Radiance, pixelCoords, vec4(outgoingRadiance, 1.0f));
}