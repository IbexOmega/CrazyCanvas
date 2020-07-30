#version 460
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_ray_tracing : enable
//#extension GL_EXT_debug_printf : enable

#include "Helpers.glsl"
#include "Defines.glsl"

struct SRadiancePayload
{
	vec3 IncomingRadiance;
};

struct SShadowPayload
{
	float Distance;
};

layout(binding = 0, set = BUFFER_SET_INDEX) uniform accelerationStructureEXT   u_TLAS;
layout(binding = 6, set = BUFFER_SET_INDEX) uniform LightsBuffer       { SLightsBuffer val; }          u_LightsBuffer;
layout(binding = 7, set = BUFFER_SET_INDEX) uniform PerFrameBuffer     { SPerFrameBuffer val; }        u_PerFrameBuffer;

layout(binding = 0, set = TEXTURE_SET_INDEX) uniform sampler2D 	                u_AlbedoAO;
layout(binding = 1, set = TEXTURE_SET_INDEX) uniform sampler2D 	                u_NormalMetallicRoughness;
layout(binding = 2, set = TEXTURE_SET_INDEX) uniform sampler2D 	                u_DepthStencil;

layout(binding = 8, set = TEXTURE_SET_INDEX, rgba8) writeonly uniform image2D   u_Radiance;

layout(location = 0) rayPayloadEXT SRadiancePayload s_RadiancePayload;
layout(location = 1) rayPayloadEXT SShadowPayload 	s_ShadowPayload;

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
	if (dot(sampledNormalMetallicRoughness, sampledNormalMetallicRoughness) < EPSILON)
	{
		return;
	}

	SLightsBuffer lightsBuffer			= u_LightsBuffer.val;
    SPerFrameBuffer perFrameBuffer   	= u_PerFrameBuffer.val;

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
	float NdotV     				= max(dot(normal, rayDirections.ViewDir), 0.0f); //Same as cosTheta

	vec3 outgoingRadiance = vec3(0.0f);

	vec3 F_0 = vec3(0.04f);
	vec3 F_0_Albedo = mix(F_0, albedo, metallic);

	//Shadow Rays
	{
		//Directional Light
		{
			//Define Shadow Ray Parameters
			const vec3 		origin 				= positions.WorldPos + normal * 0.025f;
			const vec3 		direction			= lightsBuffer.Direction.xyz;
			const uint 		rayFlags           	= gl_RayFlagsOpaqueEXT/* | gl_RayFlagsTerminateOnFirstHitEXT*/;
			const uint 		cullMask           	= 0xFF;
			const uint 		sbtRecordOffset    	= 1;
			const uint 		sbtRecordStride    	= 0;
			const uint 		missIndex          	= 1;
			const float 	Tmin              	= 0.001f;
			const float 	Tmax              	= 10000.0f;
			const int 		payload       		= 1;
		
			//Send Shadow Ray
			traceRayEXT(u_TLAS, rayFlags, cullMask, sbtRecordOffset, sbtRecordStride, missIndex, origin.xyz, Tmin, direction.xyz, Tmax, payload);

			vec3 lightDir       = normalize(lightsBuffer.Direction.xyz);
			vec3 halfway        = normalize(rayDirections.ViewDir + lightDir);
			vec3 radiance       = vec3(lightsBuffer.SpectralIntensity.rgb);

			float NdotL         = max(dot(normal, lightDir), 0.0f);
			float HdotV         = max(dot(halfway, rayDirections.ViewDir), 0.0f);

			float NDF           = Distribution(normal, halfway, roughness);
			float G             = GeometryOpt(NdotV, NdotL, roughness);
			vec3 F              = Fresnel(F_0_Albedo, HdotV);

			vec3 reflected      = F;                                                //k_S
			vec3 refracted      = (vec3(1.0f) - reflected) * (1.0f - metallic);     //k_D

			vec3 numerator      = NDF * G * F;
			float denominator   = 4.0f / NdotV * NdotL;
			vec3 specular       = numerator / max(denominator, EPSILON);

			float shadow 		= step(s_ShadowPayload.Distance, Tmin);

			outgoingRadiance += shadow * (refracted * albedo / PI + specular) * radiance * NdotL; 
		}
	}

	//Reflection Rays
	{
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
		s_RadiancePayload.IncomingRadiance = vec3(0.0f, 0.0f, 0.0f);
		traceRayEXT(u_TLAS, rayFlags, cullMask, sbtRecordOffset, sbtRecordStride, missIndex, origin.xyz, Tmin, direction.xyz, Tmax, payload);

		//Calculate how much of the incoming radiance (from the Reflection Ray) gets reflected to the camera (Fresnel equations)
		
		vec3 F_0_IndirectLightning = mix(albedo, s_RadiancePayload.IncomingRadiance, metallic);

		outgoingRadiance += 0.5f * Fresnel(F_0_IndirectLightning, NdotV);
	}

	imageStore(u_Radiance, pixelCoords, vec4(outgoingRadiance, 1.0f));
}