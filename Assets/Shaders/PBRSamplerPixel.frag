#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable

#include "Helpers.glsl"
#include "Defines.glsl"

layout(location = 0) in vec2 in_TexCoord;

layout(binding = 0, set = BUFFER_SET_INDEX) uniform PerFrameBuffer	{ SPerFrameBuffer val; }	u_PerFrameBuffer;
layout(binding = 1, set = BUFFER_SET_INDEX) uniform LightsBuffer	{ SLightsBuffer val; }		u_LightsBuffer;

layout(binding = 0, set = TEXTURE_SET_INDEX) uniform sampler2D in_Albedo_AO;
layout(binding = 1, set = TEXTURE_SET_INDEX) uniform sampler2D in_Compact_Normals;
layout(binding = 2, set = TEXTURE_SET_INDEX) uniform sampler2D in_Emission_Metallic_Roughness;
layout(binding = 3, set = TEXTURE_SET_INDEX) uniform sampler2D in_Depth;

layout(location = 0) out vec4 out_Backbuffer;

void main()
{
	SPerFrameBuffer perFrameBuffer = u_PerFrameBuffer.val;
	SLightsBuffer lightBuffer = u_LightsBuffer.val;

	// vec3 worldPos = texture(samplerLinear, input.texCoord).xyz;
	float sampledDepth = texture(in_Depth, in_TexCoord).x;
	SPositions positions = CalculatePositionsFromDepth(in_TexCoord, sampledDepth, perFrameBuffer.ProjectionInv, perFrameBuffer.ViewInv);
	vec4 albedo_AO = texture(in_Albedo_AO, in_TexCoord);

	vec4 EMR = texture(in_Emission_Metallic_Roughness, in_TexCoord);
	vec2 unpackedMetallicRoughness = unpackUnorm2x16(floatBitsToUint(EMR.w));


	vec3 N = normalize(octToDir(floatBitsToUint(texture(in_Compact_Normals, in_TexCoord).x)));
	vec3 V = normalize(perFrameBuffer.Position.xyz - positions.WorldPos);

	vec3 Lo = vec3(0.0f);
	vec3 F0 = vec3(0.04f);
	F0 = mix(F0, albedo_AO.rgb, unpackedMetallicRoughness.r);

	// Loop over lights here
	//{
		// Lightning calcs
		vec3 L = normalize(lightBuffer.DirL_Direction.xyz);
		vec3 H = normalize(V + L);

		float distance = 1.0f;
		float attenuation = 1.0f / (distance * distance);
		vec3 lightColor = lightBuffer.DirL_EmittedRadiance.rgb;
		vec3 radiance = lightColor * attenuation;
		vec3 F = Fresnel(F0 ,max(dot(V, H), 0.0));

		// float roughness
		float NDF = Distribution(N, H, unpackedMetallicRoughness.g);
		float G = Geometry(N, V, L, unpackedMetallicRoughness.g);

		// BRDF - Bidirectional reflective distribution function
		vec3 numerator = NDF * G * F;
		float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
		vec3 specular = numerator / max(denominator, 0.001);

		float NdotL = max(dot(N, L), 0.0);
	//}

	vec3 kS = F;
	vec3 kD = vec3(1.0f) - kS;
	// Nullify if metallic surface, due to zero refraction on metallic surfaces
	kD *= 1.0 - unpackedMetallicRoughness.r;
	Lo = (kD * albedo_AO.rgb / PI + specular) * radiance * NdotL;
	vec3 ambient = 0.03f * albedo_AO.rgb * albedo_AO.a;
	
	vec3 color = ambient + Lo;

	// Gamma correction
	color = color / (color + vec3(1.0f));
	color = pow(color, vec3(1.0f / GAMMA));

	out_Backbuffer = vec4(color, 1.0f);
}