#version 460
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_ray_tracing : enable
//#extension GL_EXT_debug_printf : enable

#include "Helpers.glsl"
#include "Defines.glsl"
#include "RayTracingInclude.glsl"

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
	vec4 sampledRadiance 	= imageLoad(u_Radiance, pixelCoords);

	//Unpack GBuffer
	vec3 albedo         = sampledAlbedoAO.rgb;
    vec3 normal         = CalculateNormal(sampledNormalMetallicRoughness);
    float ao            = sampledAlbedoAO.a;
    float metallic      = sampledNormalMetallicRoughness.b * 0.5f + 0.5f;
    float roughness     = abs(sampledNormalMetallicRoughness.a);

	//Define local Coordinate System
	vec3 tangent 	= vec3(0.0f);
	vec3 bitangent 	= vec3(0.0f);
	CreateCoordinateSystem(normal, tangent, bitangent);

	//Create Transformation Matrices
	mat3 localToWorld = mat3(tangent, bitangent, normal);
	mat3 worldToLocal = transpose(localToWorld);

	//Define Constants
	SPositions positions            = CalculatePositionsFromDepth(screenTexCoord, sampledDepth, perFrameBuffer.ProjectionInv, perFrameBuffer.ViewInv);
    SRayDirections rayDirections   	= CalculateRayDirections(positions.WorldPos, normal, perFrameBuffer.Position.xyz, perFrameBuffer.ViewInv);	
	float NdotV     				= abs(dot(normal, rayDirections.ViewDir)); //Same as cosTheta
	float alpha						= RoughnessToAlpha(roughness);
	vec3 w_o 						= worldToLocal * rayDirections.ViewDir;

	//Create uniform Samples
	float blueNoiseX = GoldNoise(vec3(d.x, 1.0f, 1.0f), perFrameBuffer.FrameIndex, 0.0f, 1.0f);
    float blueNoiseY = GoldNoise(vec3(d.y, 1.0f, 1.0f), perFrameBuffer.FrameIndex, 0.0f, 1.0f);
	vec4 u = texture(u_BlueNoiseLUT, vec2(blueNoiseX, blueNoiseY));
	
	vec3 L_o 				= sampledRadiance.rgb;
	float accumulation		= sampledRadiance.a;

	if (perFrameBuffer.LastView != perFrameBuffer.View)
	{
		L_o					= vec3(0.0f);
		accumulation		= 0.0f;
	}

	vec3 F_0 = vec3(0.04f);
	F_0 = mix(F_0, albedo, metallic);

	//Direct Lighting
	{
		//Directional Light
		SLightSample dirLightSample = EvalDirectionalRadiance(w_o, alpha, F_0, worldToLocal);

		if (dirLightSample.PDF > 0.0f)
		{
			//Define Shadow Ray Parameters
			const vec3 		origin 				= positions.WorldPos + normal * 0.025f;
			const vec3 		direction			= dirLightSample.SampleWorldDir;
			const uint 		rayFlags           	= gl_RayFlagsOpaqueEXT/* | gl_RayFlagsTerminateOnFirstHitEXT*/;
			const uint 		cullMask           	= 0xFF;
			const uint 		sbtRecordOffset    	= 1;
			const uint 		sbtRecordStride    	= 0;
			const uint 		missIndex          	= 1;
			const float 	Tmin              	= 0.001f;
			const float 	Tmax              	= 10000.0f;
			const int 		payload       		= 1;

			//Send Shadow Ray
			s_ShadowPayload.Distance = 0.0f;
			traceRayEXT(u_TLAS, rayFlags, cullMask, sbtRecordOffset, sbtRecordStride, missIndex, origin.xyz, Tmin, direction.xyz, Tmax, payload);

			float shadow 		= step(s_ShadowPayload.Distance, Tmin);

			L_o += shadow * albedo * dirLightSample.L_d;
		}
	}

	//Indirect Lighting
	{
		//Sample the BRDF
		SReflection reflection = Sample_f(w_o, alpha, F_0, u.xy);

		vec3 reflectionDir = localToWorld * reflection.w_i;

		if (reflection.PDF > 0.0f)
		{
			//Define Reflection Ray Parameters
			const vec3 		origin 				= positions.WorldPos + normal * 0.025f;
			const vec3 		direction			= reflectionDir;
			const uint 		rayFlags           	= gl_RayFlagsOpaqueEXT/* | gl_RayFlagsTerminateOnFirstHitEXT*/;
			const uint 		cullMask           	= 0xFF;
			const uint 		sbtRecordOffset    	= 0;
			const uint 		sbtRecordStride    	= 0;
			const uint 		missIndex          	= 0;
			const float 	Tmin              	= 0.001f;
			const float 	Tmax              	= 10000.0f;
			const int 		payload       		= 0;
			
			//Send Reflection Ray
			s_RadiancePayload.L 	= vec3(0.0f, 0.0f, 0.0f);
			traceRayEXT(u_TLAS, rayFlags, cullMask, sbtRecordOffset, sbtRecordStride, missIndex, origin.xyz, Tmin, direction.xyz, Tmax, payload);

			L_o 				+= albedo * reflection.f * s_RadiancePayload.L * reflection.CosTheta / reflection.PDF;
		}
	}

	accumulation 		+= 1.0f;

	imageStore(u_Radiance, pixelCoords, vec4(L_o, accumulation));
}