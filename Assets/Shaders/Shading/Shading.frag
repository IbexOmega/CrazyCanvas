#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_nonuniform_qualifier : enable

#include "../Defines.glsl"
#include "../Helpers.glsl"
#include "../Reflections.glsl"

layout(location = 0) in vec2 in_TexCoord;

layout(binding = 0, set = BUFFER_SET_INDEX) uniform PerFrameBuffer
{ 
	SPerFrameBuffer val; 
} u_PerFrameBuffer;
layout(binding = 1, set = BUFFER_SET_INDEX) restrict readonly buffer LightsBuffer	
{
	SLightsBuffer val; 
	SPointLight pointLights[];  
} b_LightsBuffer;

layout(binding = 0, set = TEXTURE_SET_INDEX) uniform sampler2D u_GBufferAlbedo;
layout(binding = 1, set = TEXTURE_SET_INDEX) uniform sampler2D u_GBufferAORoughMetalValid;
layout(binding = 2, set = TEXTURE_SET_INDEX) uniform sampler2D u_GBufferCompactNormal;
layout(binding = 3, set = TEXTURE_SET_INDEX) uniform sampler2D u_GBufferVelocity;
layout(binding = 4, set = TEXTURE_SET_INDEX) uniform sampler2D u_GBufferDepthStencil;

layout(binding = 5, set = TEXTURE_SET_INDEX) uniform sampler2D		u_DirLShadowMap;
layout(binding = 6, set = TEXTURE_SET_INDEX) uniform samplerCube	u_PointLShadowMap[];

layout(binding = 7, set = TEXTURE_SET_INDEX) uniform samplerCube 	u_GlobalSpecularProbe;
layout(binding = 8, set = TEXTURE_SET_INDEX) uniform samplerCube 	u_GlobalDiffuseProbe;
layout(binding = 9, set = TEXTURE_SET_INDEX) uniform sampler2D 		u_IntegrationLUT;
layout(binding = 10, set = TEXTURE_SET_INDEX) uniform sampler2D 	u_Reflections;
layout(binding = 11, set = TEXTURE_SET_INDEX) uniform sampler2D 	u_BRDF_PDF;

layout(location = 0) out vec4 out_Color;

layout(push_constant) uniform ReflectionSettings
{
	int GlossyEnabled;
	int SPP;
} pc_ReflectionSettings;

void main()
{
	SPerFrameBuffer perFrameBuffer	= u_PerFrameBuffer.val;
	SLightsBuffer lightBuffer		= b_LightsBuffer.val;

	vec3 albedo				= texture(u_GBufferAlbedo, in_TexCoord).rgb;
	vec4 aoRoughMetalValid	= texture(u_GBufferAORoughMetalValid, in_TexCoord);
	vec3 colorHDR;

	float roughness		= max(aoRoughMetalValid.g, 0.001f);

	if (aoRoughMetalValid.a < 1.0f)
	{
		float luminance = CalculateLuminance(albedo);

		//Reinhard Tone-Mapping
		vec3 colorLDR = albedo / (albedo + vec3(1.0f));

		out_Color = vec4(colorLDR, luminance);
		return;
	}
	else
	{
		float ao		= aoRoughMetalValid.r;
		float metallic	= aoRoughMetalValid.b;
		float depth 	= texture(u_GBufferDepthStencil, in_TexCoord).r;

		SPositions positions	= CalculatePositionsFromDepth(in_TexCoord, depth, perFrameBuffer.ProjectionInv, perFrameBuffer.ViewInv);
		vec3 N 					= UnpackNormal(texture(u_GBufferCompactNormal, in_TexCoord).xyz);
		
		vec3 viewVector			= perFrameBuffer.CameraPosition.xyz - positions.WorldPos;
		float viewDistance		= length(viewVector);
		vec3 V 					= normalize(viewVector);

		vec3 Lo = vec3(0.0f);
		vec3 F0 = vec3(0.04f);

		F0 = mix(F0, albedo, metallic);

		float inShadowDirLight = 0.0f;
		// Directional Light
		{
			vec3 L = normalize(lightBuffer.DirL_Direction);
			vec3 H = normalize(V + L);

			vec4 fragPosLight 		= lightBuffer.DirL_ProjView * vec4(positions.WorldPos, 1.0f);
			inShadowDirLight 		= DirShadowDepthTest(fragPosLight, N, lightBuffer.DirL_Direction, u_DirLShadowMap);
			vec3 outgoingRadiance	= lightBuffer.DirL_ColorIntensity.rgb * lightBuffer.DirL_ColorIntensity.a;
			vec3 incomingRadiance	= outgoingRadiance * (1.0f - inShadowDirLight);

			float NDF	= Distribution(N, H, roughness);
			float G		= Geometry(N, V, L, roughness);
			vec3 F		= Fresnel(F0, max(dot(V, H), 0.0f));

			vec3 nominator		= NDF * G * F;
			float denominator	= 4.0f * max(dot(N, V), 0.0f) * max(dot(N, L), 0.0f);
			vec3 specular		= nominator / max(denominator, 0.001f);

			vec3 kS = F;
			vec3 kD = vec3(1.0f) - kS;

			kD *= 1.0f - metallic;

			float NdotL = max(dot(N, L), 0.0f);

			Lo += (kD * albedo / PI + specular) * incomingRadiance * NdotL;
		}

		//Point Light Loop
		for (uint i = 0; i < uint(lightBuffer.PointLightCount); i++)
		{
			SPointLight light = b_LightsBuffer.pointLights[i];

			vec3 L = (light.Position - positions.WorldPos);
			float distance = length(L);
			L = normalize(L);
			vec3 H = normalize(V + L);
			
			float inShadow 			= PointShadowDepthTest(positions.WorldPos, light.Position, viewDistance, N, u_PointLShadowMap[light.TextureIndex], light.FarPlane);
			float attenuation		= 1.0f / (distance * distance);
			vec3 outgoingRadiance	= light.ColorIntensity.rgb * light.ColorIntensity.a;
			vec3 incomingRadiance	= outgoingRadiance * attenuation * (1.0f - inShadow);
		
			float NDF	= Distribution(N, H, roughness);
			float G		= Geometry(N, V, L, roughness);
			vec3 F		= Fresnel(F0, max(dot(V, H), 0.0f));

			vec3 nominator		= NDF * G * F;
			float denominator	= 4.0f * max(dot(N, V), 0.0f) * max(dot(N, L), 0.0f);
			vec3 specular		= nominator / max(denominator, 0.001f);

			vec3 kS = F;
			vec3 kD = vec3(1.0f) - kS;

			kD *= 1.0f - metallic;

			float NdotL = max(dot(N, L), 0.0f);

			Lo += (kD * albedo / PI + specular) * incomingRadiance * NdotL;
		}
		
		vec3 ambient;

		if (aoRoughMetalValid.g <= mix(SPECULAR_REFLECTION_REJECT_THRESHOLD, GLOSSY_REFLECTION_REJECT_THRESHOLD, float(pc_ReflectionSettings.GlossyEnabled)))
		{
			vec3 reflectionColor = texture(u_Reflections, in_TexCoord).rgb;
			vec4 BRDF_PDF = texture(u_BRDF_PDF, in_TexCoord);

			Lo += reflectionColor * BRDF_PDF.rgb / BRDF_PDF.a;
			ambient = vec3(0.03) * albedo * ao;
		}
		else
		{
			float dotNV = max(dot(N, V), 0.0f);
			vec3 F_IBL	= FresnelRoughness(F0, dotNV, roughness);
			vec3 Ks_IBL	= F_IBL;
			vec3 Kd_IBL	= 1.0f - Ks_IBL;
			Kd_IBL		*= 1.0f - metallic;
		
			vec3 irradiance		= texture(u_GlobalDiffuseProbe, N).rgb;
			vec3 IBL_Diffuse	= irradiance * albedo * Kd_IBL;
		
			const int numberOfMips = textureQueryLevels(u_GlobalSpecularProbe);
			vec3 reflection			= reflect(-V, N);
			vec3 prefiltered		= textureLod(u_GlobalSpecularProbe, reflection, roughness * float(numberOfMips)).rgb;
			vec2 integrationBRDF	= textureLod(u_IntegrationLUT, vec2(dotNV, roughness), 0).rg;
			vec3 IBL_Specular		= prefiltered * (F_IBL * integrationBRDF.x + integrationBRDF.y);
		
			ambient = (IBL_Diffuse + IBL_Specular) * ao;
		}
		
		colorHDR		= ambient + Lo;
	}

	float luminance = CalculateLuminance(colorHDR);

	//Reinhard Tone-Mapping
	vec3 colorLDR = colorHDR / (colorHDR + vec3(1.0f));

	//Gamma Correction
	vec3 finalColor = pow(colorLDR, vec3(1.0f / GAMMA));
	out_Color = vec4(finalColor, luminance);
}
