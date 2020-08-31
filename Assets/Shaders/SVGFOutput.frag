#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable
//#extension GL_EXT_debug_printf : enable

#include "Helpers.glsl"
#include "Defines.glsl"

layout(location = 0) in vec2	in_TexCoord;

layout(binding = 0, set = NO_BUFFERS_TEXTURE_SET_INDEX) uniform usampler2D 	u_LinearZ;
layout(binding = 1, set = NO_BUFFERS_TEXTURE_SET_INDEX) uniform sampler2D 	u_DirectRadiance;
layout(binding = 2, set = NO_BUFFERS_TEXTURE_SET_INDEX) uniform sampler2D 	u_IndirectRadiance;
layout(binding = 3, set = NO_BUFFERS_TEXTURE_SET_INDEX) uniform sampler2D 	u_History;
layout(binding = 4, set = NO_BUFFERS_TEXTURE_SET_INDEX) uniform sampler2D 	u_Moments;

layout(location = 0) out vec4	out_Color;
layout(location = 1) out uvec4  out_LinearZ;
layout(location = 2) out float  out_History;
layout(location = 3) out vec4   out_Moments;

void main()
{
    uvec4 sampledLinearZ                    = texture(u_LinearZ,                    in_TexCoord);
    vec4 sampledDirectRadiance              = texture(u_DirectRadiance,             in_TexCoord);
    vec4 sampledIndirectRadiance            = texture(u_IndirectRadiance,           in_TexCoord);
    float sampledHistory                    = texture(u_History,                    in_TexCoord).r;
    vec4 sampledMoments                     = texture(u_Moments,                    in_TexCoord);

    out_LinearZ                 = sampledLinearZ;
    out_History                 = sampledHistory;
    out_Moments                 = sampledMoments;

    out_Color       = vec4(GammaCorrection(sampledDirectRadiance.rgb, GAMMA), 1.0f);
}