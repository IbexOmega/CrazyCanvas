#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable
//#extension GL_EXT_debug_printf : enable

#include "SVGFAtrousCommon.glsl"

layout(location = 0) in vec2	in_TexCoord;

layout(location = 0) out vec4   out_Direct;
layout(location = 1) out vec4   out_Indirect;

void main()
{
    const int   STEP_SIZE    = 2;

    const ivec2 iPos       = ivec2(gl_FragCoord.xy);
    AtrousFilter(iPos, STEP_SIZE, out_Direct, out_Indirect);
}