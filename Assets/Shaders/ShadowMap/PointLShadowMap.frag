#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shader_draw_parameters : enable
#extension GL_GOOGLE_include_directive : enable

#include "../Defines.glsl"
#include "../Helpers.glsl"

layout(location = 0) in vec4 out_WorldPos;
layout(location = 1) in flat vec3 out_PointLightPosition;

void main()
{
    float lightDistance = length(out_WorldPos.xyz - out_PointLightPosition);

    float farplane = 10.0f;
    gl_FragDepth = lightDistance / farplane;
}
