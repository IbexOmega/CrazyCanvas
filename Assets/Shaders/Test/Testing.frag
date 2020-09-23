#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable

#include "../Defines.glsl"

layout(binding = 0,     set = TEXTURE_SET_INDEX, rgba8) restrict uniform image2D  u_A;

layout(location = 0) out vec4 out_Color;

void main()
{
    out_Color = vec4(1.0f, 0.0f, 1.0f, 1.0f);    
}
