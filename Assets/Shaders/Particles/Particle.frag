#version 450 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_nonuniform_qualifier : enable

#include "../Defines.glsl"
#include "../Helpers.glsl"

layout(location = 0) in vec2 in_TexCoords;
layout(location = 1) in flat uint in_AtlasIndex;
layout(location = 2) in vec4 in_EmitterColor;
layout(location = 3) in vec3 in_WorldPos;
layout(location = 4) in mat3 in_TBN;

layout (binding = 0, set = 0) uniform PerFrameBuffer
{
	SPerFrameBuffer PerFrameBuffer;
} u_PerFrameBuffer;

layout(binding = 0, set = 1) uniform sampler2D 		u_TextureAtlases[];
layout(binding = 1, set = 1) uniform sampler2D 		u_DirLShadowMap;
layout(binding = 2, set = 1) uniform samplerCube 	u_PointLShadowMap[];

layout(binding = 4, set = 2) restrict readonly buffer LightsBuffer	
{
	SLightsBuffer val; 
	SPointLight pointLights[];  
} b_LightsBuffer;

layout(location = 0) out vec4 out_ParticleImage;

void main()
{
	SPerFrameBuffer perFrameBuffer	= u_PerFrameBuffer.PerFrameBuffer;
	SLightsBuffer lightBuffer		= b_LightsBuffer.val;
	
	vec4 color = texture(u_TextureAtlases[in_AtlasIndex], in_TexCoords);
	if(color.a < 0.9f) discard;
	
	vec3 colorHDR;
	vec3 normal;
	{
		vec3 albedo		= in_EmitterColor.rgb;
		float ao		= 0.0f;
		float roughness	= 0.1f;
		float metallic	= 0.0f;

		vec3 worldPos    		= in_WorldPos;
		vec3 N 					= normalize(in_TBN * color.xyz);
		vec3 viewVector			= perFrameBuffer.CameraPosition.xyz - worldPos;
		float viewDistance		= length(viewVector);
		vec3 V 					= normalize(viewVector);

		normal = N;

		vec3 Lo = vec3(0.0f);
		vec3 F0 = vec3(0.04f);

		F0 = mix(F0, albedo, metallic);
		// Directional Light
		{
			vec3 L = normalize(lightBuffer.DirL_Direction);
			vec3 H = normalize(V + L);

			vec4 fragPosLight 		= lightBuffer.DirL_ProjView * vec4(worldPos, 1.0);
			// float inShadow 			= DirShadowDepthTest(fragPosLight, N, lightBuffer.DirL_Direction, u_DirLShadowMap);
			vec3 outgoingRadiance    = lightBuffer.DirL_ColorIntensity.rgb * lightBuffer.DirL_ColorIntensity.a;
			vec3 incomingRadiance    = outgoingRadiance;// * (1.0 - inShadow);

			float NDF   = Distribution(N, H, roughness);
			float G     = Geometry(N, V, L, roughness);
			vec3 F      = Fresnel(F0, max(dot(V, H), 0.0f));

			vec3 nominator      = NDF * G * F;
			float denominator   = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0f);
			vec3 specular       = nominator / max(denominator, 0.001f);

			vec3 kS = F;
			vec3 kD = vec3(1.0f) - kS;

			kD *= 1.0 - metallic;

			float NdotL = max(dot(N, L), 0.0f);

			Lo += (kD * albedo / PI + specular) * incomingRadiance * NdotL;
		}

		// Point Light Loop
		for (uint i = 0; i < uint(lightBuffer.PointLightCount); i++)
		{
			SPointLight light = b_LightsBuffer.pointLights[i];

			vec3 L = (light.Position - worldPos);
			float distance = length(L);
			L = normalize(L);
			vec3 H = normalize(V + L);
			
			float inShadow 			= PointShadowDepthTest(worldPos, light.Position, viewDistance, N, u_PointLShadowMap[light.TextureIndex], light.FarPlane);
			float attenuation   	= 1.0f / (distance * distance);
			vec3 outgoingRadiance    = light.ColorIntensity.rgb * light.ColorIntensity.a;
			vec3 incomingRadiance    = outgoingRadiance * attenuation * (1.0 - inShadow);
		
			float NDF   = Distribution(N, H, roughness);
			float G     = Geometry(N, V, L, roughness);
			vec3 F      = Fresnel(F0, max(dot(V, H), 0.0f));

			vec3 nominator      = NDF * G * F;
			float denominator   = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0f);
			vec3 specular       = nominator / max(denominator, 0.001f);

			vec3 kS = F;
			vec3 kD = vec3(1.0f) - kS;

			kD *= 1.0 - metallic;

			float NdotL = max(dot(N, L), 0.0f);

			Lo += (kD * albedo / PI + specular) * incomingRadiance * NdotL;
		}
		
		vec3 ambient    = 0.4f * albedo * ao;
		colorHDR      	= ambient + Lo;
	}

	float luminance = CalculateLuminance(colorHDR);

	//Reinhard Tone-Mapping
	vec3 colorLDR = colorHDR / (colorHDR + vec3(1.0f));

	//Gamma Correction
	vec3 finalColor = pow(colorLDR, vec3(1.0f / GAMMA));


	out_ParticleImage = vec4(finalColor, 1.0);
}