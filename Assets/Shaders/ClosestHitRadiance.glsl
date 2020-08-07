#version 460
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_ray_tracing : enable

#include "Helpers.glsl"
#include "Defines.glsl"
#include "RayTracingInclude.glsl"

layout(location = 0) rayPayloadInEXT SRadiancePayload s_RadiancePayload;
layout(location = 1) rayPayloadEXT SShadowPayload 	s_ShadowPayload;

hitAttributeEXT vec3 attribs;

void main() 
{
    SRayHitDescription hitDescription = CalculateHitData(attribs, gl_InstanceCustomIndexEXT, gl_PrimitiveID, gl_ObjectToWorldEXT);

	vec3 hitPos = gl_WorldRayOriginEXT + normalize(gl_WorldRayDirectionEXT) * gl_HitTEXT;

    vec3 albedo 	= pow(  texture(u_SceneAlbedoMaps[hitDescription.MaterialIndex],    hitDescription.TexCoord).rgb, vec3(GAMMA));
	float metallic 	= 		texture(u_SceneMetallicMaps[hitDescription.MaterialIndex],	hitDescription.TexCoord).r;
	float roughness = 		texture(u_SceneRoughnessMaps[hitDescription.MaterialIndex],	hitDescription.TexCoord).r;

	mat3 worldToLocal = transpose(hitDescription.LocalToWorld);

	vec3 w_o 		= worldToLocal * -gl_WorldRayDirectionEXT;

	float alpha		= RoughnessToAlpha(roughness);

	vec3 F_0 = vec3(0.04f);
	F_0 = mix(F_0, albedo, metallic);

	vec3 L_o = vec3(0.0f);

	//Emitted Light
	{
		vec3 L_e = vec3(0.0f);

		L_o += L_e;
	}

	//Direct Lighting
	{
		//Directional Light
		SLightSample dirLightSample = EvalDirectionalRadiance(w_o, alpha, F_0, worldToLocal);

		if (dirLightSample.PDF > 0.0f)
		{
			//Define Shadow Ray Parameters
			const vec3 		origin 				= hitPos + hitDescription.Normal * 0.025f;
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


    s_RadiancePayload.L += L_o;
}