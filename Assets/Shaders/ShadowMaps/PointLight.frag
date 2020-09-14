#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shader_draw_parameters : enable
#extension GL_GOOGLE_include_directive : enable

layout( push_constant ) uniform PushConstants {
    uint Iteration;
    float Near;
    float Far;
} pc;

layout(location = 0) in vec4 in_WorldPosition;
layout(location = 1) in flat vec4 in_PointLightPosition;

void main()
{
    float distanceToLight = length(in_WorldPosition.xyz - in_PointLightPosition.xyz);
    gl_FragDepth = distanceToLight / 25.0f;
}