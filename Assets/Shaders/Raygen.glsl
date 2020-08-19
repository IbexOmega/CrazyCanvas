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

	//Directional Light
	SLightSample dirLightSample = EvalDirectionalRadiance(w_o, s_RadiancePayload.Albedo, s_RadiancePayload.Metallic, s_RadiancePayload.Roughness, worldToLocal);

	if (dirLightSample.PDF > 0.0f && dot(dirLightSample.L_d, dirLightSample.L_d) > EPSILON)
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
	vec4 sampledAlbedoAO    		= texture(u_AlbedoAO, screenTexCoord);
	float sampledDepth      		= texture(u_DepthStencil, screenTexCoord).r;
	vec4 sampledDirectRadiance 		= imageLoad(u_DirectRadiance, pixelCoords);
	vec4 sampledIndirectRadiance 	= imageLoad(u_IndirectRadiance, pixelCoords);

	//Unpack GBuffer
	vec3 albedo         = sampledAlbedoAO.rgb;
    vec3 normal         = CalculateNormal(sampledNormalMetallicRoughness);
    float ao            = sampledAlbedoAO.a;
    float metallic      = abs(sampledNormalMetallicRoughness.b);
    float roughness     = max(EPSILON, abs(sampledNormalMetallicRoughness.a));
	bool emissive		= sampledNormalMetallicRoughness.b > 0.0f;

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

	//Define Sample Variables
	uint randomSeed = perFrameBuffer.RandomSeed + pixelCoords.x * gl_LaunchSizeEXT.x + pixelCoords.y;
    uvec3 randomSeedPoint = uvec3(randomSeed, randomSeed >> 10, randomSeed >> 20);
	ivec3 blueNoiseSize = textureSize(u_BlueNoiseLUT, 0);
	
	vec3 L_o_Direct 		= sampledDirectRadiance.rgb;
	vec3 L_o_Indirect 		= sampledIndirectRadiance.rgb;
	float accumulation		= sampledDirectRadiance.a;

	if (perFrameBuffer.PrevView != perFrameBuffer.View)
	{
		L_o_Direct			= vec3(0.0f);
		L_o_Indirect		= vec3(0.0f);
		accumulation		= 0.0f;
	}

	s_RadiancePayload.ScatterPosition	= positions.WorldPos + normal * RAY_NORMAL_OFFSET;
	s_RadiancePayload.Albedo			= albedo;
	s_RadiancePayload.Metallic			= metallic;
	s_RadiancePayload.Roughness			= roughness;
	s_RadiancePayload.Emissive			= emissive;
	s_RadiancePayload.Distance			= 1.0f;
	s_RadiancePayload.LocalToWorld 		= localToWorld;

	const int maxBounces 				= 8;
	const int russianRouletteStart		= 3;
	const int numSamplesPerFrame		= maxBounces * 3;

	const float MIN_ROUGHNESS_DELTA_DISTRIBUTION = EPSILON * 2.0f;

	vec3 throughput  					= vec3(1.0f);

	for (int b = 0; b < maxBounces; b++)
	{
		vec3 L 		= vec3(0.0f);

		int baseB 	= b * 3;
		vec3 u = vec3( 	GenerateSample(baseB + 0, randomSeedPoint, numSamplesPerFrame, blueNoiseSize),
				 		GenerateSample(baseB + 1, randomSeedPoint, numSamplesPerFrame, blueNoiseSize),
				 		GenerateSample(baseB + 2, randomSeedPoint, numSamplesPerFrame, blueNoiseSize));
		bool isSpecular = s_RadiancePayload.Roughness < MIN_ROUGHNESS_DELTA_DISTRIBUTION;

		//Emissive Surface
		if (s_RadiancePayload.Emissive)
		{
			L 			+= throughput * s_RadiancePayload.Albedo * 10.0f;
			//accumulation 	+= 1.0f;
		}

		//Direct Lighting (next event estimation)
		if (!isSpecular) //Since specular distributions are described by a delta distribution, lights have 0 probability of contributing to this reflection
		{
			//L 			+= SampleLights(w_o, worldToLocal, throughput);
			//accumulation 	+= 1.0f;
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

		//Russian Roulette
		{
			if (b > russianRouletteStart)
			{
				float p = max(throughput.r, max(throughput.g, throughput.b));

				if (u.z > p)
				{
					break;
				}

				throughput *= 1.0f / p;
			}
		}

		//Write to Direct or Indirect L_o
		if (b < 2) //b == 0 is no bounce, b == 1 is one bounce (direct lighting)
		{
			L_o_Direct 		+= L;
		}
		else
		{
			L_o_Indirect 	+= L;
		}
	}

	accumulation 	+= 1.0f;

	//Direct Lighting (next event estimation)
	// if (s_RadiancePayload.Distance > 0.0f && s_RadiancePayload.Roughness > MIN_ROUGHNESS_DELTA_DITRIBUTION_LIGHTS)
	// {
	// 	L_o 			+= SampleLights(w_o, worldToLocal, throughput);
	// 	accumulation 	+= 1.0f;
	// }

	imageStore(u_DirectRadiance, 	pixelCoords, vec4(L_o_Direct, 	accumulation));
	imageStore(u_IndirectRadiance, 	pixelCoords, vec4(L_o_Indirect, 1.0f));
}