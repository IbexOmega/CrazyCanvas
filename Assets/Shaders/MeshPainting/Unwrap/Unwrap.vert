#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shader_draw_parameters : enable
#extension GL_GOOGLE_include_directive : enable

#include "../../Defines.glsl"


layout(binding = 0, set = BUFFER_SET_INDEX) uniform PerFrameBuffer              { SPerFrameBuffer val; }    u_PerFrameBuffer;

layout(binding = 0, set = NO_TEXTURES_DRAW_SET_INDEX) restrict readonly buffer Vertices     { SVertex val[]; }          b_Vertices;
layout(binding = 1, set = NO_TEXTURES_DRAW_SET_INDEX) restrict readonly buffer Instances    { SInstance val[]; }        b_Instances;

layout(location = 0) out vec3 out_WorldPosition;

void main()
{
    SVertex vertex                              = b_Vertices.val[gl_VertexIndex];
    SInstance instance                          = b_Instances.val[gl_InstanceIndex];
    SPerFrameBuffer perFrameBuffer              = u_PerFrameBuffer.val;

    vec4 worldPosition      = /*instance.Transform * */vec4(vertex.Position.xyz, 1.0f);
    //vec4 prevWorldPosition  = instance.PrevTransform * vec4(vertex.Position.xyz, 1.0f);

    out_WorldPosition       = worldPosition.xyz;

    const float CAPTURE_SIZE = 1.0f;
    const vec3 UNWRAP_LOCATION = vec3(0.0f, 0.f, 0.f);

    //vec4 out_ClipPosition        = perFrameBuffer.Projection * perFrameBuffer.View * worldPosition;
    vec2 texCoord = vec2(vertex.TexCoord.x, vertex.TexCoord.y);
    texCoord = (texCoord*2.f - 1.f)*CAPTURE_SIZE;
    gl_Position = vec4(vec3(texCoord, 0.f)+UNWRAP_LOCATION, 1.f); //vec4(vec3((vertex.TexCoord.xy-0.5)*CAPTURE_SIZE, 0)+UNWRAP_LOCATION - worldPosition.xyz, 1.f);
}