#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable
//#extension GL_EXT_debug_printf : enable

#include "Helpers.glsl"
#include "Defines.glsl"

layout(location = 0) in vec2	in_TexCoord;

layout(binding = 0, set = NO_BUFFERS_TEXTURE_SET_INDEX) uniform sampler2D 	u_AlbedoAO;
layout(binding = 1, set = NO_BUFFERS_TEXTURE_SET_INDEX) uniform sampler2D 	u_NormalMetallicRoughness;
layout(binding = 2, set = NO_BUFFERS_TEXTURE_SET_INDEX) uniform sampler2D 	u_DepthStencil;
layout(binding = 3, set = NO_BUFFERS_TEXTURE_SET_INDEX) uniform sampler2D 	u_LinearZ;
layout(binding = 4, set = NO_BUFFERS_TEXTURE_SET_INDEX) uniform sampler2D 	u_DirectRadiance;
layout(binding = 5, set = NO_BUFFERS_TEXTURE_SET_INDEX) uniform sampler2D 	u_IndirectRadiance;
layout(binding = 6, set = NO_BUFFERS_TEXTURE_SET_INDEX) uniform sampler2D 	u_Moments;
layout(binding = 7, set = NO_BUFFERS_TEXTURE_SET_INDEX) uniform sampler2D 	u_History;

layout(location = 0) out vec4	out_Color;
layout(location = 1) out vec4   out_Albedo_AO;
layout(location = 2) out vec4   out_Normals_Metall_Rough;
layout(location = 3) out vec4   out_LinearZ;
layout(location = 4) out vec4   out_Direct_Radiance;
layout(location = 5) out vec4   out_Indirect_Radiance;
layout(location = 6) out vec4   out_Moments;

void main()
{
    vec4 sampledAlbedoAO                    = texture(u_AlbedoAO,                   in_TexCoord);
    vec4 sampledNormalMetallicRoughness     = texture(u_NormalMetallicRoughness,    in_TexCoord);
    vec4 sampledLinearZ                     = texture(u_NormalMetallicRoughness,    in_TexCoord);
    float sampledDepth                      = texture(u_DepthStencil,               in_TexCoord).r;
    vec4 sampledDirectRadiance              = texture(u_DirectRadiance,             in_TexCoord);
    vec4 sampledIndirectRadiance            = texture(u_IndirectRadiance,           in_TexCoord);
    vec4 sampledMoments                     = texture(u_Moments,                    in_TexCoord);

    out_Albedo_AO               = sampledAlbedoAO;
    out_Normals_Metall_Rough    = sampledNormalMetallicRoughness;
    out_LinearZ                 = sampledLinearZ;
    gl_FragDepth                = sampledDepth;
    out_Direct_Radiance         = sampledDirectRadiance;
    out_Indirect_Radiance       = sampledIndirectRadiance;
    out_Moments                 = sampledMoments;

    //Skybox
	if (dot(sampledNormalMetallicRoughness, sampledNormalMetallicRoughness) < EPSILON)
	{
		out_Color = vec4(0.529f, 0.808, 0.922f, 1.0f);
		return;
	}

    float history = texture(u_History, in_TexCoord).r;

    vec3 colorHDR   = (sampledDirectRadiance.rgb + sampledIndirectRadiance.rgb) / history;
    vec3 colorLDR   = ToneMap(colorHDR, GAMMA);

    out_Color       = vec4(history, history, history, 1.0f);
}