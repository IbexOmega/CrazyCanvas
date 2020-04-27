#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable

#include "helpers.glsl"

struct SLightsBuffer
{
    vec4    Direction;
	vec4    SpectralIntensity;
};

struct SPerFrameBuffer
{
    mat4 Projection;
	mat4 View;
	mat4 LastProjection;
	mat4 LastView;
	mat4 ViewInv;
	mat4 ProjectionInv;
	vec4 Position;
	vec4 Right;
	vec4 Up;
};

layout(location = 0) in vec2	in_TexCoord;

layout(binding = 0, set = 0) uniform sampler2D 	u_AlbedoAO;
layout(binding = 1, set = 0) uniform sampler2D 	u_NormalMetallicRoughness;
layout(binding = 2, set = 0) uniform sampler2D 	u_DepthStencil;
layout(binding = 3, set = 0) uniform sampler2D 	u_Radiance;

layout(binding = 0, set = 1) uniform LightsBuffer     { SLightsBuffer val; }        u_LightsBuffer;
layout(binding = 1, set = 1) uniform PerFrameBuffer   { SPerFrameBuffer val; }      u_PerFrameBuffer;

layout(location = 0) out vec4	out_Color;

void main()
{
    vec4 sampledAlbedoAO                    = texture(u_AlbedoAO,                   in_TexCoord);
    vec4 sampledNormalMetallicRoughness     = texture(u_NormalMetallicRoughness,    in_TexCoord);

    //Skybox
	if (dot(sampledNormalMetallicRoughness, sampledNormalMetallicRoughness) < EPSILON)
	{
		out_Color = vec4(sampledAlbedoAO.rgb, 1.0f);
		return;
	}

    vec4 sampledDepthStencil                = texture(u_DepthStencil,               in_TexCoord);
    vec4 sampledRadiance                    = texture(u_Radiance,                   in_TexCoord);

    vec3 albedo         = sampledAlbedoAO.rgb;
    vec3 normal         = CalculateNormal(sampledNormalMetallicRoughness);
    float ao            = sampledAlbedoAO.a;
    float metallic      = sampledNormalMetallicRoughness.b;
    float roughness     = abs(sampledNormalMetallicRoughness.a);
    
    vec3 radiance = sampledRadiance.rgb;

    SLightsBuffer lightsBuffer                  = u_LightsBuffer.val;
    SPerFrameBuffer perFrameBuffer              = u_PerFrameBuffer.val;

    SPositions positions            = CalculatePositionsFromDepth(in_TexCoord, sampledDepthStencil.r, perFrameBuffer.ProjectionInv, perFrameBuffer.ViewInv);
    vec3 viewDir = normalize(perFrameBuffer.Position.xyz - positions.WorldPos);
    float NdotV     = max(dot(normal, viewDir), 0.0f);

    vec3 F_0 = vec3(0.04f);
    F_0 = mix(F_0, albedo, metallic);

    vec3 L_o = vec3(0.0f);

    //Directional Light
    {
        vec3 lightDir       = normalize(lightsBuffer.Direction.xyz);
        vec3 halfway        = normalize(viewDir + lightDir);
        vec3 radiance       = vec3(lightsBuffer.SpectralIntensity.rgb);

        float NdotL         = max(dot(normal, lightDir), 0.0f);
        float HdotV         = max(dot(halfway, viewDir), 0.0f);

        float NDF           = Distribution(normal, halfway, roughness);
        float G             = GeometryOpt(NdotV, NdotL, roughness);
        vec3 F              = Fresnel(F_0, HdotV);

        vec3 reflected      = F;                                                //k_S
        vec3 refracted      = (vec3(1.0f) - reflected) * (1.0f - metallic);     //k_D

        vec3 numerator      = NDF * G * F;
        float denominator   = 4.0f / NdotV * NdotL;
        vec3 specular       = numerator / max(denominator, EPSILON);

        L_o += (refracted * albedo / PI + specular) * radiance * NdotL; 
    }

    vec3 ambient    = vec3(0.03f) * albedo * ao;
    vec3 colorHDR   = ambient + L_o;
    
    vec3 colorLDR   = ToneMap(colorHDR, GAMMA);

    out_Color = vec4(colorLDR, 1.0f);
}