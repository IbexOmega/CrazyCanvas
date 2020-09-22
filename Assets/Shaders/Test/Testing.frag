#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable

layout(location = 0) out vec4 out_Color;

void main()
{
    out_Color = vec4(1.0f, 0.0f, 1.0f, 1.0f);    
}
