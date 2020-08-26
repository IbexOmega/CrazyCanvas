#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable
//#extension GL_EXT_debug_printf : enable

#include "Helpers.glsl"
#include "Defines.glsl"

layout(location = 0) in vec2	in_TexCoord;

layout(binding = 0, set = NO_BUFFERS_TEXTURE_SET_INDEX) uniform sampler2D 	u_DirectRadiance;
layout(binding = 1, set = NO_BUFFERS_TEXTURE_SET_INDEX) uniform sampler2D 	u_IndirectRadiance;
layout(binding = 2, set = NO_BUFFERS_TEXTURE_SET_INDEX) uniform sampler2D 	u_History;

layout(location = 0) out vec4	out_Color;

void main()
{
    vec4 sampledDirectRadiance              = texture(u_DirectRadiance,   in_TexCoord);
    vec4 sampledIndirectRadiance            = texture(u_IndirectRadiance, in_TexCoord);
    float history                           = texture(u_History, in_TexCoord).r;

    vec3 colorHDR   = (sampledDirectRadiance.rgb + sampledIndirectRadiance.rgb) / history;
    vec3 colorLDR   = GammaCorrection(colorHDR, GAMMA);

    out_Color       = vec4(colorLDR, 1.0f);
}