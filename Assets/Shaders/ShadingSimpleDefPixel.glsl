#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable
//#extension GL_EXT_debug_printf : enable

#include "Helpers.glsl"
#include "Defines.glsl"

struct SLightsBuffer
{
    vec4    Direction;
	vec4    SpectralIntensity;
};

layout(location = 0) in vec2	in_TexCoord;

layout(binding = 0, set = NO_BUFFERS_TEXTURE_SET_INDEX) uniform sampler2D 	u_AlbedoAO;

layout(location = 0) out vec4	out_Color;

void main()
{
    out_Color  = texture(u_AlbedoAO,                   in_TexCoord);
}