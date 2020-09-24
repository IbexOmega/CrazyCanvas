#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shader_draw_parameters : enable
#extension GL_GOOGLE_include_directive : enable

layout(location = 0) in vec3        out_WorldPosition;

layout(location = 0) out vec4       out_MaskTexture;

void main()
{
    out_MaskTexture = vec4(out_WorldPosition, 1.f);
}
