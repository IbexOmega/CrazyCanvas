#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shader_draw_parameters : enable
#extension GL_GOOGLE_include_directive : enable

#include "../Defines.glsl"

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

layout(location = 0) in flat uint in_MaterialSlot[];
layout(location = 1) in vec3 in_WorldPosition[];
layout(location = 2) in vec3 in_Normal[];
layout(location = 3) in vec3 in_Tangent[];
layout(location = 4) in vec3 in_Bitangent[];
layout(location = 5) in vec2 in_TexCoord[];
layout(location = 6) in vec4 in_ClipPosition[];
layout(location = 7) in vec4 in_PrevClipPosition[];
layout(location = 8) in vec4 in_PaintInfo4[];
layout(location = 9) in float in_PaintDist[];
layout(location = 10) in float in_VertDist[];

layout(location = 0) out flat uint out_MaterialSlot;
layout(location = 1) out vec3 out_WorldPosition;
layout(location = 2) out vec3 out_Normal;
layout(location = 3) out vec3 out_Tangent;
layout(location = 4) out vec3 out_Bitangent;
layout(location = 5) out vec2 out_TexCoord;
layout(location = 6) out vec4 out_ClipPosition;
layout(location = 7) out vec4 out_PrevClipPosition;
layout(location = 8) out vec4 out_PaintInfo4;
layout(location = 9) out float out_PaintDist;
layout(location = 10) out vec3 out_VertDist;

void BuildVertex(in uint index)
{
    gl_Position = gl_in[index].gl_Position;
    out_MaterialSlot = in_MaterialSlot[index];
    out_WorldPosition = in_WorldPosition[index];
    out_Normal = in_Normal[index];
    out_Tangent = in_Tangent[index];
    out_Bitangent = in_Bitangent[index];
    out_TexCoord = in_TexCoord[index];
    out_ClipPosition = in_ClipPosition[index];
    out_PrevClipPosition = in_PrevClipPosition[index];
    out_PaintInfo4 = in_PaintInfo4[index];
    out_PaintDist = in_PaintDist[index];
    out_VertDist = vec3(float(index==0), float(index==1), float(index==2));
}

void main()
{
    BuildVertex(0);
    EmitVertex();

    BuildVertex(1);
    EmitVertex();

    BuildVertex(2);
    EmitVertex();

    EndPrimitive();
}