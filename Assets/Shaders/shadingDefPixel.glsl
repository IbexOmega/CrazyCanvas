#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable

#include "helpers.glsl"

layout(location = 0) in vec2	in_TexCoord;

layout(binding = 0) uniform sampler2D 	u_AlbedoAO;
layout(binding = 1) uniform sampler2D 	u_NormalMetallicRoughness;
layout(binding = 2) uniform sampler2D 	u_DepthStencil;

layout(location = 0) out vec4	out_Color;

void main()
{
    vec4 sampledAlbedoAO             = texture(u_AlbedoAO,                   in_TexCoord);
    vec4 sampledMetallicRougness     = texture(u_NormalMetallicRoughness,    in_TexCoord);
    vec4 sampledDepthStencil         = texture(u_DepthStencil,               in_TexCoord);

    vec3 albedo = sampledAlbedoAO.rgb;
    
    out_Color = vec4(albedo, 1.0f);
}
