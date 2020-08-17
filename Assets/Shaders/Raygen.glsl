#version 460
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_ray_tracing : enable
//#extension GL_EXT_debug_printf : enable

#include "Helpers.glsl"
#include "Defines.glsl"
#include "RayTracingInclude.glsl"

layout(location = 0) rayPayloadEXT SRadiancePayload s_RadiancePayload;
layout(location = 1) rayPayloadEXT SShadowPayload 	s_ShadowPayload;

vec3 SampleLights(vec3 w_o, mat3 worldToLocal, vec3 throughput)
{
	vec3 L_d = vec3(0.0f);

	const float MIN_ROUGHNESS_DELTA_DITRIBUTION_LIGHTS = EPSILON * 2.0f;
	if (s_RadiancePayload.Roughness > MIN_ROUGHNESS_DELTA_DITRIBUTION_LIGHTS) //Since specular distributions are described by a delta distribution, lights have 0 probability of contributing to this reflection
	{
		//Directional Light
		SLightSample dirLightSample = EvalDirectionalRadiance(w_o, s_RadiancePayload.Albedo, s_RadiancePayload.Metallic, s_RadiancePayload.Roughness, worldToLocal);

		if (dirLightSample.PDF > 0.0f)
		{
			//Define Shadow Ray Parameters
			const vec3 		origin 				= s_RadiancePayload.ScatterPosition;
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
			traceRayEXT(u_TLAS, rayFlags, cullMask, sbtRecordOffset, sbtRecordStride, missIndex, origin.xyz, Tmin, direction.xyz, Tmax, payload);

			float shadow 		= step(s_ShadowPayload.Distance, Tmin);

			L_d += throughput * shadow * dirLightSample.L_d;
		}
	}

	return L_d;
}

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
    float roughness     = max(EPSILON, abs(sampledNormalMetallicRoughness.a));

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
	vec3 world_w_o 					= rayDirections.ViewDir;
	vec3 w_o						= worldToLocal * world_w_o;

	//Create uniform Samples
	float blueNoiseX = GoldNoise(vec3(d.x, 1.0f, 1.0f), perFrameBuffer.FrameIndex, 0.0f, 1.0f);
    float blueNoiseY = GoldNoise(vec3(d.y, 1.0f, 1.0f), perFrameBuffer.FrameIndex, 0.0f, 1.0f);
	
	vec3 L_o 				= sampledRadiance.rgb;
	float accumulation		= sampledRadiance.a;

	if (perFrameBuffer.LastView != perFrameBuffer.View)
	{
		L_o					= vec3(0.0f);
		accumulation		= 0.0f;
	}

	s_RadiancePayload.ScatterPosition	= positions.WorldPos + normal * 0.025f;
	s_RadiancePayload.Albedo			= albedo;
	s_RadiancePayload.Metallic			= metallic;
	s_RadiancePayload.Roughness			= roughness;
	s_RadiancePayload.Distance			= 1.0f;
	s_RadiancePayload.LocalToWorld 		= localToWorld;

	int maxBounces 				= 8;
	vec3 throughput  			= vec3(1.0f);

	for (int b = 0; b < maxBounces; b++)
	{
		vec4 u = texture(u_BlueNoiseLUT[b], vec2(blueNoiseX, blueNoiseY));		

		//Emitted light
		{
		
		}

		//Direct Lighting (next event estimation)
		{
			L_o 			+= SampleLights(w_o, worldToLocal, throughput);
			accumulation 	+= 1.0f;
		}

		//Indirect Lighting
		{
			//Sample the BRDF
			SReflection reflection = Sample_f(w_o, s_RadiancePayload.Albedo,  s_RadiancePayload.Metallic, s_RadiancePayload.Roughness, u.xy);

			vec3 reflectionDir = s_RadiancePayload.LocalToWorld * reflection.w_i;

			if (reflection.PDF > 0.0f)
			{
				throughput  		*= reflection.f * reflection.CosTheta / reflection.PDF;
				world_w_o			= -reflectionDir;

				//Define Reflection Ray Parameters
				const vec3 		origin 				= s_RadiancePayload.ScatterPosition;
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
				traceRayEXT(u_TLAS, rayFlags, cullMask, sbtRecordOffset, sbtRecordStride, missIndex, origin.xyz, Tmin, direction.xyz, Tmax, payload);
		
				if (s_RadiancePayload.Distance <= Tmin)
				{
					break;
				}

				worldToLocal 	= transpose(s_RadiancePayload.LocalToWorld);
				w_o				= worldToLocal * world_w_o;
			}
			else
			{
				s_RadiancePayload.ScatterPosition	= vec3(0.0f);
				s_RadiancePayload.Albedo			= vec3(0.0f);
				s_RadiancePayload.Metallic			= 0.0f;
				s_RadiancePayload.Roughness			= 0.0f;
				s_RadiancePayload.Distance			= 0.0f;
				s_RadiancePayload.LocalToWorld 		= mat3(1.0f);

				break;
			}
		}
	}

	//Direct Lighting (next event estimation)
	if (s_RadiancePayload.Distance > 0.0f)
	{
		L_o 			+= SampleLights(w_o, worldToLocal, throughput);
		accumulation 	+= 1.0f;
	}

	imageStore(u_Radiance, pixelCoords, vec4(L_o, accumulation));
}