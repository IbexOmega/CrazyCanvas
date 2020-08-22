#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable
//#extension GL_EXT_debug_printf : enable

#include "Helpers.glsl"
#include "Defines.glsl"

layout(location = 0) in vec2	in_TexCoord;

layout(binding = 0, set = NO_BUFFERS_TEXTURE_SET_INDEX) uniform sampler2D 	        u_DirectRadiance;
layout(binding = 1, set = NO_BUFFERS_TEXTURE_SET_INDEX) uniform sampler2D 	        u_IndirectRadiance;
layout(binding = 2, set = NO_BUFFERS_TEXTURE_SET_INDEX) uniform sampler2D 	        u_Moments;
layout(binding = 3, set = NO_BUFFERS_TEXTURE_SET_INDEX) uniform sampler2D 	        u_History;
layout(binding = 4, set = NO_BUFFERS_TEXTURE_SET_INDEX) uniform sampler2D 	        u_CompNormDepth;

layout(location = 0) out vec4	out_Direct;
layout(location = 1) out vec4	out_Indirect;

void main()
{
    const ivec2 iPos    = ivec2(gl_FragCoord.xy);
    vec3 direct         = texelFetch(u_DirectRadiance,   iPos, 0).rgb;
    vec3 indirect       = texelFetch(u_IndirectRadiance, iPos, 0).rgb;

    out_Direct      = vec4(direct,      1.0f);
    out_Indirect    = vec4(indirect,    1.0f);
}