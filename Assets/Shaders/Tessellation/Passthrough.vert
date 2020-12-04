#version 460
#extension GL_GOOGLE_include_directive : enable

#include "../Defines.glsl"

layout(binding = 0, set = 0) restrict buffer InVertices    { SVertex val[]; }  b_InVertices;

layout(binding = 0, set = 1) restrict writeonly buffer OutVertices				{ SVertex val[]; }	b_OutVertices;

layout(location = 0) out float out_PosW;
layout(location = 1) out vec4 out_Normal;
layout(location = 2) out vec4 out_Tangent;
layout(location = 3) out vec4 out_TexCoord;

void main()
{
    SVertex vertex = b_InVertices.val[gl_VertexIndex]; 
    out_PosW = vertex.Position.w;
    out_Normal = vertex.Normal;
    out_Tangent = vertex.Tangent;
    out_TexCoord = vertex.TexCoord;
    gl_Position = vec4(vertex.Position.xyz, 1.f);
}