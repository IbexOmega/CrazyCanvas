#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shader_draw_parameters : enable
#extension GL_GOOGLE_include_directive : enable

#include "../Defines.glsl"

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

layout(binding = 0, set = 1) restrict writeonly buffer OutVertices				{ SVertex val[]; }	b_OutVertices;
layout(binding = 1, set = 1) restrict writeonly buffer OutIndices		        { uint val[]; }		b_OutIndices;

layout(location = 0) in float in_PosW[];
layout(location = 1) in vec4 in_Normal[];
layout(location = 2) in vec4 in_Tangent[];
layout(location = 3) in vec4 in_TexCoord[];

// Remove this...
layout(location = 0) out float out_PosW;
layout(location = 1) out vec4 out_Normal;
layout(location = 2) out vec4 out_Tangent;
layout(location = 3) out vec4 out_TexCoord;

void main()
{
    // write to buffers;
    // Use atomicAdd to get the previous index in the buffer.
}