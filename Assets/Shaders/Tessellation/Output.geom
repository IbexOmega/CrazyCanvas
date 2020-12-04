#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shader_draw_parameters : enable
#extension GL_GOOGLE_include_directive : enable

#include "../Defines.glsl"

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

// IN
layout(binding = 1, set = 0) restrict buffer CalculationData				
{
    mat4 ScaleMatrix;
    uint PrimitiveCounter;
    vec3 Padding;
}	b_CalculationData;

// Out
layout(binding = 0, set = 1) restrict writeonly buffer OutVertices				{ SVertex val[]; }	b_OutVertices;
layout(binding = 1, set = 1) restrict writeonly buffer OutIndices		        { uint val[]; }		b_OutIndices;

layout(location = 0) in float in_PosW[];
layout(location = 1) in vec4 in_Normal[];
layout(location = 2) in vec4 in_Tangent[];
layout(location = 3) in vec4 in_TexCoord[];

void main()
{
    // write to buffers;
    // uint primitiveIndex = atomicAdd(b_AtomicCounter.PrimitiveCounter, 1);
    const uint TRI_VERTEX_COUNT = 3;
    uint primitiveIndex = atomicAdd(b_CalculationData.PrimitiveCounter, 1);
    for (int i = 0; i < TRI_VERTEX_COUNT; i++)
    {
        SVertex outputVertex;
        outputVertex.Position               = vec4(gl_in[i].gl_Position);
        outputVertex.Normal                 = vec4(in_Normal[i]);
        outputVertex.Tangent                = vec4(in_Tangent[i]);
        outputVertex.TexCoord               = vec4(in_TexCoord[i]);
        b_OutVertices.val[(primitiveIndex * TRI_VERTEX_COUNT) + i]      = outputVertex;
        b_OutIndices.val[(primitiveIndex * TRI_VERTEX_COUNT) + i]       = (primitiveIndex * TRI_VERTEX_COUNT) + i;
    }
}