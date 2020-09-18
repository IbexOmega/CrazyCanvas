#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable

#include "../Defines.glsl"
#include "../Helpers.glsl"

layout(location = 0) in vec2 in_TexCoord;

layout(binding = 0, set = BUFFER_SET_INDEX) uniform PerFrameBuffer  { SPerFrameBuffer val; } u_PerFrameBuffer;
layout(binding = 1, set = BUFFER_SET_INDEX) restrict readonly buffer LightsBuffer	
{
	SLightsBuffer val; 
	SPointLight pointLights[];  
} b_LightsBuffer;

layout(binding = 0, set = TEXTURE_SET_INDEX) uniform sampler2D 	u_Albedo;
layout(binding = 1, set = TEXTURE_SET_INDEX) uniform sampler2D 	u_AORoughMetal;
layout(binding = 2, set = TEXTURE_SET_INDEX) uniform sampler2D 	u_CompactNormals;
layout(binding = 3, set = TEXTURE_SET_INDEX) uniform sampler2D 	u_Velocity;
layout(binding = 4, set = TEXTURE_SET_INDEX) uniform sampler2D 	u_DepthStencil;

layout(location = 0) out vec4 out_Color;

void main()
{
	SPerFrameBuffer perFrameBuffer  = u_PerFrameBuffer.val;
	SLightsBuffer lightBuffer       = b_LightsBuffer.val;

	float sampledDepth      = texture(u_DepthStencil, in_TexCoord).r;
	SPositions positions    = CalculatePositionsFromDepth(in_TexCoord, sampledDepth, perFrameBuffer.ProjectionInv, perFrameBuffer.ViewInv);

	vec3 albedo     = texture(u_Albedo, in_TexCoord).rgb;
	vec4 aoRoughMetal = texture(u_AORoughMetal, in_TexCoord);

	vec3 N = normalize(OctToDir(texture(u_CompactNormals, in_TexCoord).xy));
	vec3 V = normalize(perFrameBuffer.Position.xyz - positions.WorldPos);

	vec3 Lo = vec3(0.0f);
	vec3 F0 = vec3(0.04f);

	F0 = mix(F0, albedo, aoRoughMetal.b);

	//Point Light Loop
	for (uint i = 0; i < lightBuffer.PointLightCount; ++i)
	{
		SPointLight light = b_LightsBuffer.pointLights[i];

		vec3 L = normalize(vec3(0.0f, 1.0f, 0.0f));
		vec3 H = normalize(V + L);

		float distance      = length(light.Position - positions.WorldPos);
		float attenuation   = 1.0f / (distance * distance);
		vec3 outgoingRadiance    = light.ColorIntensity.rgb * light.ColorIntensity.a;
		vec3 incomingRadiance    = outgoingRadiance * attenuation;
	
		float NDF   = Distribution(N, H, aoRoughMetal.g);
		float G     = Geometry(N, V, L, aoRoughMetal.g);
		vec3 F      = Fresnel(F0, max(dot(V, H), 0.0f));

		vec3 nominator      = NDF * G * F;
		float denominator   = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0f);
		vec3 specular       = nominator / max(denominator, 0.001f);

		vec3 kS = F;
		vec3 kD = vec3(1.0f) - kS;

		kD *= 1.0 - aoRoughMetal.b;

		float NdotL = max(dot(N, L), 0.0f);

		Lo += (kD * albedo / PI + specular) * incomingRadiance * NdotL;
	}
	
	vec3 ambient    = 0.03f * albedo * aoRoughMetal.r;
	vec3 color      = ambient + Lo;

	color = color / (color + vec3(1.0f));
	color = pow(color, vec3(1.0f / GAMMA));

	out_Color = vec4(color, 1.0f);
}