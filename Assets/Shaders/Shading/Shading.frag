#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable

#include "../Defines.glsl"
#include "../Helpers.glsl"

layout(location = 0) in vec2 in_TexCoord;

layout(binding = 0, set = BUFFER_SET_INDEX) uniform PerFrameBuffer  { SPerFrameBuffer val; }    u_PerFrameBuffer;
//layout(binding = 1, set = BUFFER_SET_INDEX) uniform LightsBuffer	{ SLightsBuffer val; }		u_LightsBuffer;

layout(binding = 0, set = TEXTURE_SET_INDEX) uniform sampler2D 	u_GBufferPosition;
layout(binding = 1, set = TEXTURE_SET_INDEX) uniform sampler2D 	u_GBufferAlbedo;
layout(binding = 2, set = TEXTURE_SET_INDEX) uniform sampler2D 	u_GBufferAORoughMetalValid;
layout(binding = 3, set = TEXTURE_SET_INDEX) uniform sampler2D 	u_GBufferCompactNormal;
layout(binding = 4, set = TEXTURE_SET_INDEX) uniform sampler2D 	u_GBufferVelocity;

layout(location = 0) out vec4 out_Color;

void main()
{
    SPerFrameBuffer perFrameBuffer  = u_PerFrameBuffer.val;
    //SLightsBuffer lightBuffer       = u_LightsBuffer.val;

    vec3 albedo             = texture(u_GBufferAlbedo, in_TexCoord).rgb;
    vec4 aoRoughMetalValid  = texture(u_GBufferAORoughMetalValid, in_TexCoord);

    if (aoRoughMetalValid.a < 1.0f)
    {
        vec3 color = albedo / (albedo + vec3(1.0f));
        color = pow(color, vec3(1.0f / GAMMA));
        out_Color = vec4(color, 1.0f);
        return;
    }

    vec3 worldPos           = texture(u_GBufferPosition, in_TexCoord).rgb;

    vec3 N = normalize(OctToDir(texture(u_GBufferCompactNormal, in_TexCoord).xy));
    vec3 V = normalize(perFrameBuffer.CameraPosition.xyz - worldPos);

    vec3 Lo = vec3(0.0f);
    vec3 F0 = vec3(0.04f);

    F0 = mix(F0, albedo, aoRoughMetalValid.b);

    //Loop
    //for (uint i = 0; i < lightBuffer.AreaLightCount(); ++i)
    //{
        vec3 L = normalize(vec3(0.0f, 1.0f, 0.0f));
        vec3 H = normalize(V + L);

        float distance      = 1.0f;
        float attenuation   = 1.0f / (distance * distance);
        vec3 lightColor     = vec3(1.0f, 1.0f, 1.0f);
        vec3 radiance       = lightColor * attenuation;
    
        float NDF   = Distribution(N, H, aoRoughMetalValid.g);
        float G     = Geometry(N, V, L, aoRoughMetalValid.g);
        vec3 F      = Fresnel(F0, max(dot(V, H), 0.0f));

        vec3 nominator      = NDF * G * F;
        float denominator   = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0f);
        vec3 specular       = nominator / max(denominator, 0.001f);

        vec3 kS = F;
        vec3 kD = vec3(1.0f) - kS;

        kD *= 1.0 - aoRoughMetalValid.b;

        float NdotL = max(dot(N, L), 0.0f);

        Lo += (kD * albedo / PI + specular) * radiance * NdotL;
    //}
    
    vec3 ambient    = 0.03f * albedo * aoRoughMetalValid.r;
    vec3 color      = ambient + Lo;

    color = color / (color + vec3(1.0f));
    color = pow(color, vec3(1.0f / GAMMA));

    out_Color = vec4(color, 1.0f);
}