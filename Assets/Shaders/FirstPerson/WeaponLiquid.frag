#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shader_draw_parameters : enable
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_nonuniform_qualifier : enable

#include "../Defines.glsl"
#include "../Helpers.glsl"

layout(binding = 2, set = BUFFER_SET_INDEX) readonly buffer PaintMaskColors		{ vec4 val[]; }					b_PaintMaskColor;
#include "../MeshPaintHelper.glsl"

// Pushconstants
layout(push_constant) uniform PushConstants
{
	layout(offset = 72) uint TeamIndex;
	layout(offset = 76) uint IsWater; 
} u_PC;

layout(location = 0) in vec3		in_WorldPosition;
layout(location = 1) in vec3		in_Normal;
layout(location = 2) in vec3		in_Tangent;
layout(location = 3) in vec3		in_Bitangent;
layout(location = 4) in vec2		in_TexCoord;
layout(location = 5) in vec4		in_ClipPosition;
layout(location = 6) in vec4		in_PrevClipPosition;
layout(location = 7) in flat uint	in_InstanceIndex;
layout(location = 8) in vec3 		in_ViewDirection;
layout(location = 9) in vec3        in_Position;
	
layout(binding = 0, set = BUFFER_SET_INDEX) uniform PerFrameBuffer              { SPerFrameBuffer val; }        u_PerFrameBuffer;
layout(binding = 1, set = BUFFER_SET_INDEX) readonly buffer MaterialParameters 	{ SMaterialParameters val[]; }	b_MaterialParameters; // Not used
layout(binding = 3, set = BUFFER_SET_INDEX) restrict readonly buffer LightsBuffer	
{
	SLightsBuffer val; 
	SPointLight pointLights[];  
} b_LightsBuffer;

layout(binding = 0, set = TEXTURE_SET_INDEX) uniform sampler2D u_AlbedoMaps[]; // Not used
layout(binding = 1, set = TEXTURE_SET_INDEX) uniform sampler2D u_NormalMaps[]; // Not used
layout(binding = 2, set = TEXTURE_SET_INDEX) uniform sampler2D u_CombinedMaterialMaps[]; // Not used
layout(binding = 3, set = TEXTURE_SET_INDEX) uniform sampler2D u_DepthStencil;

layout(location = 0) out vec4 out_Color;

void main()
{

	vec3 normal		= normalize(in_Normal);
	vec3 tangent	= normalize(in_Tangent);
	vec3 bitangent	= normalize(in_Bitangent);
	vec2 texCoord	= in_TexCoord;

	mat3 TBN = mat3(tangent, bitangent, normal);

	vec3 sampledAlbedo = u_PC.IsWater == 1 ? vec3(0.f, 0.f, 1.f) : b_PaintMaskColor.val[u_PC.TeamIndex].rgb;

    // Tells the user how much paint is in the container. A limit of 0.75 is 75% filled. 
    float limit = 0.5f; //u_PC.WaveX + u_PC.WaveZ;

    // Use the texture coordinate to find the height at this pixel from the bottom of the container to the top.
    const float distTexCoord = 0.087f;
    const float midTexCoord = 0.24f;
    const float minTexCoord = midTexCoord-distTexCoord;
    const float maxTexCoord = midTexCoord+distTexCoord;
    float filling = in_Position.y;//(clamp(texCoord.y, minTexCoord, maxTexCoord)-minTexCoord)/(maxTexCoord-minTexCoord);
    float isLiquid = step(1.f-limit, filling);
    
    // Remove the pixels which are not part of the liquid.
    if(isLiquid < 0.5f)
        discard;

    // Color the side with the paint color and the top with a lighter paint color.
    vec3 color = gl_FrontFacing ? sampledAlbedo : clamp(sampledAlbedo + vec3(1.f)*.7f, vec3(0.f), vec3(1.f));
	out_Color = vec4(color, 1.f);
	return;

	// Get weapon albedo
	vec3 storedAlbedo = pow(sampledAlbedo, vec3(GAMMA));

	// PBR
	SPerFrameBuffer perFrameBuffer	= u_PerFrameBuffer.val;
	SLightsBuffer lightBuffer		= b_LightsBuffer.val;

	vec3 storedMaterial	= vec3(1.f, 1.f, 0.f);
	vec4 aoRoughMetalValid	= vec4(storedMaterial, 1.0f);
	
	float ao		= aoRoughMetalValid.r;
	float roughness	= 1.0f-aoRoughMetalValid.g; // TODO fix need to invert
	float metallic	= aoRoughMetalValid.b;
	float depth 	= texture(u_DepthStencil, in_TexCoord).r;

	vec3 N 					= normalize(normal);
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