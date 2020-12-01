#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shader_draw_parameters : enable
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_nonuniform_qualifier : enable

#include "../Defines.glsl"
#include "../Helpers.glsl"

layout(binding = 2, set = BUFFER_SET_INDEX) readonly buffer PaintMaskColors		{ vec4 val[]; }					b_PaintMaskColor;
#include "../MeshPaintHelper.glsl"

layout(location = 0) in flat uint	in_MaterialSlot;
layout(location = 1) in vec3		in_WorldPosition;
layout(location = 2) in vec3		in_Normal;
layout(location = 3) in vec3		in_Tangent;
layout(location = 4) in vec3		in_Bitangent;
layout(location = 5) in vec2		in_TexCoord;
layout(location = 6) in vec4		in_ClipPosition;
layout(location = 7) in vec4		in_PrevClipPosition;
layout(location = 8) in flat uint	in_ExtensionIndex;
layout(location = 9) in flat uint	in_InstanceIndex;
layout(location = 10) in vec3 		in_ViewDirection;
layout(location = 11) in vec4		in_PaintInfo4;
layout(location = 12) in float 		in_PaintDist;
layout(location = 13) in vec3 		in_LocalPosition;
	
layout(binding = 0, set = BUFFER_SET_INDEX) uniform PerFrameBuffer
{ 
	SPerFrameBuffer val; 
} u_PerFrameBuffer;
layout(binding = 1, set = BUFFER_SET_INDEX) readonly buffer MaterialParameters 	{ SMaterialParameters val[]; }	b_MaterialParameters;
layout(binding = 3, set = BUFFER_SET_INDEX) restrict readonly buffer LightsBuffer	
{
	SLightsBuffer val; 
	SPointLight pointLights[];  
} b_LightsBuffer;

layout(binding = 0, set = TEXTURE_SET_INDEX) uniform sampler2D u_AlbedoMaps[];
layout(binding = 1, set = TEXTURE_SET_INDEX) uniform sampler2D u_NormalMaps[];
layout(binding = 2, set = TEXTURE_SET_INDEX) uniform sampler2D u_CombinedMaterialMaps[];
layout(binding = 3, set = TEXTURE_SET_INDEX) uniform sampler2D u_DepthStencil;

layout(location = 0) out vec4 out_Color;

void main()
{
	// out_Color = vec4(1.0f, 1.0f, 0.0f, 0.5);
	// return;

	vec3 normal		= normalize(in_Normal);
	vec3 tangent	= normalize(in_Tangent);
	vec3 bitangent	= normalize(in_Bitangent);
	vec2 texCoord	= in_TexCoord;

	mat3 TBN = mat3(tangent, bitangent, normal);

	vec3 sampledAlbedo				= texture(u_AlbedoMaps[in_MaterialSlot],			texCoord).rgb;

	vec3 sampledNormal				= texture(u_NormalMaps[in_MaterialSlot],			texCoord).rgb;
	vec3 sampledCombinedMaterial	= texture(u_CombinedMaterialMaps[in_MaterialSlot],	texCoord).rgb;

	vec3 shadingNormal		= normalize((sampledNormal * 2.0f) - 1.0f);
	shadingNormal			= normalize(TBN * normalize(shadingNormal));

	SMaterialParameters materialParameters = b_MaterialParameters.val[in_MaterialSlot];
	uint packedPaintInfo = 0;
	float dist = 1.f;
	GetVec4ToPackedPaintInfoAndDistance(in_LocalPosition, in_PaintInfo4, in_PaintDist, packedPaintInfo, dist);
	SPaintDescription paintDescription = InterpolatePaint(TBN, in_LocalPosition, tangent, bitangent, packedPaintInfo, dist);

	shadingNormal = mix(shadingNormal, paintDescription.Normal + shadingNormal * 0.2f, paintDescription.Interpolation);

	vec2 currentNDC		= (in_ClipPosition.xy / in_ClipPosition.w) * 0.5f + 0.5f;
	vec2 prevNDC		= (in_PrevClipPosition.xy / in_PrevClipPosition.w) * 0.5f + 0.5f;

	float shouldPaint = float(step(1, packedPaintInfo));
	bool isPainted = (shouldPaint > 0.5f) && (paintDescription.Interpolation > 0.001f);

	// Darken back faces like inside of painted legs
	float backSide = 1.0f - step(0.0f, dot(in_ViewDirection, shadingNormal));
	paintDescription.Albedo = mix(paintDescription.Albedo, paintDescription.Albedo*0.8, backSide);

	// Get weapon albedo
	vec3 storedAlbedo = pow(materialParameters.Albedo.rgb * sampledAlbedo, vec3(GAMMA));

	// Apply paint
	storedAlbedo = mix(storedAlbedo, paintDescription.Albedo, paintDescription.Interpolation);

	// PBR
	SPerFrameBuffer perFrameBuffer	= u_PerFrameBuffer.val;
	SLightsBuffer lightBuffer		= b_LightsBuffer.val;

	vec3 storedMaterial	= vec3(
								materialParameters.AO * sampledCombinedMaterial.b, 
								mix(materialParameters.Roughness * sampledCombinedMaterial.r, paintDescription.Roughness, paintDescription.Interpolation), 
								materialParameters.Metallic * sampledCombinedMaterial.g * float(paintDescription.Interpolation == 0.0f));
	vec4 aoRoughMetalValid	= vec4(storedMaterial, 1.0f);
	
	float ao		= aoRoughMetalValid.r;
	float roughness	= 1.0f-aoRoughMetalValid.g; // TODO fix need to invert
	float metallic	= aoRoughMetalValid.b;
	float depth 	= texture(u_DepthStencil, in_TexCoord).r;

	vec3 N 					= shadingNormal;
	vec3 viewVector			= perFrameBuffer.CameraPosition.xyz - in_WorldPosition;
	float viewDistance		= length(viewVector);
	vec3 V 					= normalize(viewVector);

	vec3 Lo = vec3(0.0f);
	vec3 F0 = vec3(0.06f);

	F0 = mix(F0, storedAlbedo, metallic);

	// Directional Light
	{
		vec3 L = normalize(lightBuffer.DirL_Direction);
		vec3 H = normalize(V + L);

		vec4 fragPosLight 		= lightBuffer.DirL_ProjView * vec4(in_WorldPosition, 1.0);
		vec3 outgoingRadiance    = lightBuffer.DirL_ColorIntensity.rgb * lightBuffer.DirL_ColorIntensity.a;
		vec3 incomingRadiance    = outgoingRadiance;

		float NDF   = Distribution(N, H, roughness);
		float G     = Geometry(N, V, L, roughness);
		vec3 F      = Fresnel(F0, max(dot(V, H), 0.0f));

		vec3 nominator      = NDF * G * F;
		float denominator   = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0f);
		vec3 specular       = nominator / max(denominator, 0.001f);

		vec3 kS = F;
		vec3 kD = vec3(1.0f) - kS;

		kD *= 1.0 - metallic;

		float NdotL = max(dot(N, L), 0.05f);

		Lo += (kD * storedAlbedo / PI + specular) * incomingRadiance * NdotL;
	}

	//Point Light Loop
	for (uint i = 0; i < uint(lightBuffer.PointLightCount); i++)
	{
		SPointLight light = b_LightsBuffer.pointLights[i];

		vec3 L = (light.Position - in_WorldPosition);
		float distance = length(L);
		L = normalize(L);
		vec3 H = normalize(V + L);
		
		float attenuation   	= 1.0f / (distance * distance);
		vec3 outgoingRadiance    = light.ColorIntensity.rgb * light.ColorIntensity.a;
		vec3 incomingRadiance    = outgoingRadiance * attenuation;
	
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

		Lo += (kD * storedAlbedo / PI + specular) * incomingRadiance * NdotL;
	}

	vec3 colorHDR = 0.03f * ao * storedAlbedo + Lo;

	// Reinhard Tone-Mapping
	vec3 colorLDR = colorHDR / (colorHDR + vec3(1.0f));

	// Gamma Correction
	vec3 finalColor = pow(colorLDR, vec3(1.0f / GAMMA));

	// Transparent team players
	//float alpha = isPainted ? 1.0f : 0.65f;
	
	out_Color = vec4(finalColor, 1.0f);
}