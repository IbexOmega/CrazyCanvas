#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable
//#extension GL_EXT_debug_printf : enable

#include "SVGFAtrousCommon.glsl"

layout(location = 0) in vec2	in_TexCoord;

layout(binding = 4, set = NO_BUFFERS_TEXTURE_SET_INDEX) uniform sampler2D 	u_DirectAlbedo;
layout(binding = 5, set = NO_BUFFERS_TEXTURE_SET_INDEX) uniform sampler2D 	u_IndirectAlbedo;

layout(location = 0) out vec4   out_Direct;
layout(location = 1) out vec4   out_Indirect;

void main()
{
    const int   STEP_SIZE    = 8;

    const ivec2 iPos       = ivec2(gl_FragCoord.xy);
    AtrousFilter(iPos, STEP_SIZE, out_Direct, out_Indirect);

    // do the demodulation in the last iteration to save memory bandwidth
    vec4 directAlbedo   = texelFetch(u_DirectAlbedo, iPos, 0);
    vec4 indirectAlbedo = texelFetch(u_IndirectAlbedo, iPos, 0);

    out_Direct = (out_Direct * directAlbedo + out_Indirect * indirectAlbedo);
}