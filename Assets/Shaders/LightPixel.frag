#version 450
#extension GL_ARB_separate_shader_objects : enable

#include "Helpers.glsl"

#define MAX_POINT_LIGHTS 	4
#define MAX_REFLECTION_MIPS 6.0f

layout(location = 0) in vec2	in_TexCoord;
layout(location = 0) out vec4	out_Color;

layout (constant_id = 0) const int RAY_TRACING_ENABLED = 0;

layout(binding = 1) uniform sampler2D 	u_Albedo;
layout(binding = 2) uniform sampler2D 	u_Normal;
layout(binding = 3) uniform sampler2D 	u_Velocity;
layout(binding = 4) uniform sampler2D 	u_Depth;
layout(binding = 5) uniform samplerCube u_IrradianceMap;
layout(binding = 6) uniform samplerCube u_EnvironmentMap;
layout(binding = 7) uniform sampler2D 	u_BrdfLUT;
layout(binding = 8) uniform sampler2D 	u_Radiance;
layout(binding = 9) uniform sampler2D 	u_Glossy;

struct PointLight
{
	vec4 Color;
	vec4 Position;
};

layout (binding = 0) uniform PerFrameBuffer
{
	mat4 Projection;
	mat4 View;
	mat4 LastProjection;
	mat4 LastView;
	mat4 InvView;
	mat4 InvProjection;
	vec4 Position;
	vec4 Right;
	vec4 Up;
} g_PerFrame;

layout (binding = 10) uniform LightBuffer
{
	PointLight lights[MAX_POINT_LIGHTS];
} u_Lights;

/*
	Get position from depth
*/
vec3 WorldPositionFromDepth(vec2 texCoord, float depth)
{
	vec4 clipspace = vec4((texCoord * 2.0f) - 1.0f, depth, 1.0f);
	vec4 viewSpace = g_PerFrame.InvProjection * clipspace;
	viewSpace = viewSpace / viewSpace.w;

	vec4 worldPosition = g_PerFrame.InvView * viewSpace;
	return worldPosition.xyz;
}

vec4 ColorWrite(vec3 finalColor)
{
	vec3 result = finalColor / (finalColor + vec3(0.75f));
	result = pow(result, vec3(1.0f / GAMMA));
	return vec4(result, 1.0f);
}

void main()
{
	vec2 texCoord = in_TexCoord;

	//Unpack
	vec4 	sampledAlbedo 		= texture(u_Albedo, texCoord);
	vec4 	sampledNormal 		= texture(u_Normal, texCoord);
	vec4 	sampledVelocity 	= texture(u_Velocity, texCoord);
	float 	sampledDepth 		= texture(u_Depth, texCoord).r;

	vec3 albedo 		= sampledAlbedo.rgb;
	vec3 worldPosition 	= WorldPositionFromDepth(texCoord, sampledDepth);

	//Get normal from z = sqrt(1 - (x^2 + y^2))), negative metallic = negative normal.z
	vec3 normal;
	normal.xy 	= sampledNormal.xy;
	normal.z 	= sqrt(1.0f - dot(normal.xy, normal.xy));
	if (sampledNormal.a < 0)
	{
		normal.z = -normal.z;
	}
	normal = normalize(normal);

	//Just skybox
	if (abs(normal.z) == 1.0f)
	{
	    out_Color = ColorWrite(albedo);
		return;
	}

	float ao 			= sampledAlbedo.a;
	float roughness 	= abs(sampledNormal.a);
	float metallic		= sampledNormal.z;

	vec3 cameraPosition = g_PerFrame.Position.xyz;
	vec3 viewDir 		= normalize(cameraPosition - worldPosition);
	vec3 reflection 	= reflect(-viewDir, normal);

	vec3 f0 = vec3(0.04f);
	f0 = mix(f0, albedo, metallic);

	float metallicFactor = 1.0f - metallic;

	//Radiance from light
	vec3 L0 = vec3(0.0f);
	if (RAY_TRACING_ENABLED == 1)
	{
		L0 = texture(u_Radiance, texCoord).rgb;
	}
	else
	{
		for (int i = 0; i < MAX_POINT_LIGHTS; i++)
		{
			vec3 lightPosition 	= u_Lights.lights[i].Position.xyz;
			vec3 lightColor 	= u_Lights.lights[i].Color.rgb;

			vec3 lightDirection = (lightPosition - worldPosition);
			float distance 		= length(lightDirection);
			float attenuation 	= 1.0f / (distance * distance);

			lightDirection 	= normalize(lightDirection);
			vec3 halfVector = normalize(viewDir + lightDirection);

			vec3 radiance = lightColor * attenuation;

			vec3 f 		= Fresnel(f0, clamp(dot(halfVector, viewDir), 0.0f, 1.0f));
			float ndf 	= Distribution(normal, halfVector, roughness);
			float g 	= Geometry(normal, viewDir, lightDirection, roughness);

			float denom 	= (4.0f * max(dot(normal, viewDir), 0.0f) * max(dot(normal, lightDirection), 0.0f)) + 0.0001f;
			vec3 specular   = (ndf * g * f) / denom;

			//Take 1.0f minus the incoming radiance to get the diffuse (Energy conservation)
			vec3 diffuse = (vec3(1.0f) - f) * metallicFactor;

			L0 += ((diffuse * (albedo / PI)) + specular) * radiance * max(dot(normal, lightDirection), 0.0f);
		}
	}

	//Irradiance from surroundings
	vec3 irradiance = texture(u_IrradianceMap, normal).rgb;
	vec3 diffuse 	= irradiance * albedo;
	vec3 f 			= FresnelRoughness(f0, clamp(dot(normal, viewDir), 0.0f, 1.0f), roughness);
	vec3 kDiffuse 	= (vec3(1.0f) - f) * metallicFactor;

	vec3 prefilteredColor = vec3(0.0f);

	if (RAY_TRACING_ENABLED == 1)
	{
		vec4 centerRayTracedGlossy = texture(u_Glossy, texCoord);
		//vec4 rayTracedGlossy = blur(u_Glossy, centerRayTracedGlossy, texCoord, vec2(0.0f, 1.0f), centerRayTracedGlossy.a);//texture(u_RayTracingResult, texCoord);
		prefilteredColor = centerRayTracedGlossy.rgb;
	}
	else
	{
		prefilteredColor = textureLod(u_EnvironmentMap, reflection, roughness * MAX_REFLECTION_MIPS).rgb;
	}

	vec2 envBRDF 	= texture(u_BrdfLUT, vec2(max(dot(normal, viewDir), 0.0f), roughness)).rg;
	vec3 specular	= prefilteredColor * (f * envBRDF.x + envBRDF.y);
	vec3 ambient 	= ((kDiffuse * diffuse) + specular) * ao;

	vec3 finalColor = ambient + L0;

    out_Color = ColorWrite(finalColor);
	//out_Color = vec4(prefilteredColor, 1.0f);
	//out_Color.rgb = vec3(abs(sampledNormal.a));
	//out_Color.rgb = texture(u_Velocity, texCoord).rgb * 10.0f;
}