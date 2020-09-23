#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shader_draw_parameters : enable
#extension GL_GOOGLE_include_directive : enable

#include "../Defines.glsl"

layout(binding = 0, set = BUFFER_SET_INDEX) uniform PerFrameBuffer              { SPerFrameBuffer val; }    u_PerFrameBuffer;

layout(binding = 0, set = NO_TEXTURES_DRAW_SET_INDEX) restrict readonly buffer Vertices     { SVertex val[]; }          b_Vertices;
layout(binding = 1, set = NO_TEXTURES_DRAW_SET_INDEX) restrict readonly buffer Instances    { SInstance val[]; }        b_Instances;

void main()
{
    SVertex vertex                              = b_Vertices.val[gl_VertexIndex];
    SInstance instance                          = b_Instances.val[gl_InstanceIndex];
    SPerFrameBuffer perFrameBuffer              = u_PerFrameBuffer.val;

    vec4 worldPosition      = instance.Transform * vec4(vertex.Position.xyz, 1.0f);

    gl_Position = perFrameBuffer.Projection * perFrameBuffer.View * worldPosition;
}