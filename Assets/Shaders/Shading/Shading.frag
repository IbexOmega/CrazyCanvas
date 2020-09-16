#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable

#include "../Defines.glsl"
#include "../Helpers.glsl"

layout(location = 0) in vec2 in_TexCoord;

layout(binding = 0, set = BUFFER_SET_INDEX) uniform PerFrameBuffer  { SPerFrameBuffer val; }    u_PerFrameBuffer;
layout(binding = 1, set = BUFFER_SET_INDEX) uniform LightsBuffer	{ SLightsBuffer val; }		u_LightsBuffer;

layout(binding = 0, set = TEXTURE_SET_INDEX) uniform sampler2D 	u_Albedo;
layout(binding = 1, set = TEXTURE_SET_INDEX) uniform sampler2D 	u_AORoughMetal;
layout(binding = 2, set = TEXTURE_SET_INDEX) uniform sampler2D 	u_CompactNormals;
layout(binding = 3, set = TEXTURE_SET_INDEX) uniform sampler2D 	u_Velocity;
layout(binding = 4, set = TEXTURE_SET_INDEX) uniform sampler2D 	u_DepthStencil;

layout(location = 0) out vec4 out_Color;

void main()
{
    SPerFrameBuffer perFrameBuffer  = u_PerFrameBuffer.val;
    SLightsBuffer lightBuffer       = u_LightsBuffer.val;

    float sampledDepth      = texture(u_DepthStencil, in_TexCoord).x;
    SPositions positions    = CalculatePositionsFromDepth(in_TexCoord, sampledDepth, perFrameBuffer.ProjectionInv, perFrameBuffer.ViewInv);

    vec3 albedo     = texture(u_Albedo, in_TexCoord).rgb;
    float albedo_AO = texture(u_AORoughMetal, in_TexCoord).r;
    float roughness = texture(u_AORoughMetal, in_TexCoord).g;
    float metallic  = texture(u_AORoughMetal, in_TexCoord).b;

    vec3 N = normalize(octToDir(floatBitsToUint(texture(u_CompactNormals, in_TexCoord).x)));
    vec3 V = normalize(perFrameBuffer.Position.xyz - positions.WorldPos);

    vec3 Lo = vec3(0.0f);
    vec3 F0 = vec3(0.04f);

    F0 = mix(F0, albedo, metallic);

    //Loop
    for (int i = 0; i < lightBuffer.size(); ++i)
    {
        vec3 L = normalize(lightBuffer.DirL_Direction.xyz);
        vec3 H = normalize(V + L);

        float distance      = 1.0f;
        float attenuation   = 1.0f / (distance * distance);
        vec3 lightColor     = lightBuffer.DirL_EmittedRadiance.rgb;
        vec3 radiance       = lightColor * attenuation;
    
        float NDF   = Distribution(N, H, roughness);
        float G     = Geometry(N, V, L, roughness);
        vec3 F      = Fresnel(F0, max(dot(V, H), 0.0));

        vec3 numerator      = NDF * G * F;
        float denominator   = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
        vec3 specular       = numerator / max(denominator, 0.001);

        vec3 kS = F;
        vec3 kD = vec3(1.0f) - kS;

        kD *= 1.0 - metallic;

        float NdotL = max(dot(N, L), 0.0);

        Lo += (kD * albedo / PI + specular) * radiance * NdotL;
    }
    
    vec3 ambient    = 0.03f * albedo_AO * albedo_AO;
    vec3 color      = ambient + Lo;

    color = color / (color * vec3(1.0f));
    color = pow(color, vec3(1.0 / GAMMA));

    out_Color = vec4(color, 1.0f);
}