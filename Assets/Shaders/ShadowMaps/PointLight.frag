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

void main()
{
    vec3 pointLightPos = vec3(0.0f, 2.0f, 0.0f);

    float distanceToLight = length(in_WorldPosition.xyz - pointLightPos);
    gl_FragDepth = distanceToLight / 25.0f;
}