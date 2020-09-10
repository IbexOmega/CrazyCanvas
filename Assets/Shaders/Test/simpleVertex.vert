#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shader_draw_parameters : enable
#extension GL_GOOGLE_include_directive : enable

#include "..\Defines.glsl"

layout(binding = 0, set = BUFFER_SET_INDEX) restrict readonly buffer Vertices               { SVertex val[]; }              b_Vertices;
layout(binding = 1, set = BUFFER_SET_INDEX) restrict readonly buffer Indices                { uint val[]; }                 b_Indices;
layout(binding = 2, set = BUFFER_SET_INDEX) restrict readonly buffer PrimaryInstances       { SPrimaryInstance val[]; }     b_PrimaryInstances;
layout(binding = 3, set = BUFFER_SET_INDEX) uniform PerFrameBuffer                          { SPerFrameBuffer val; }        u_PerFrameBuffer;

layout(location = 0) out vec3 out_VertexPos;

void main()
{
    SPrimaryInstance primaryInstance            = b_PrimaryInstances.val[gl_InstanceIndex];
    SVertex vertex                              = b_Vertices.val[gl_VertexIndex];
    SPerFrameBuffer perFrameBuffer              = u_PerFrameBuffer.val;

    out_VertexPos = vertex.Position.xyz;

    //Calculate a 4x4 matrix so that we calculate the correct w for worldPosition
    mat4 transform;
	transform[0] = vec4(primaryInstance.Transform[0]);
	transform[1] = vec4(primaryInstance.Transform[1]);
	transform[2] = vec4(primaryInstance.Transform[2]);
	transform[3] = vec4(0.0f, 0.0f, 0.0f, 1.0f);
    transform = transpose(transform);

    vec4 worldPosition      = transform * vec4(out_VertexPos, 1.0f);

    mat4 rotationMatrix = perFrameBuffer.View;
    rotationMatrix[3] = vec4(0.f, 0.f, 0.f, 1.0f);

    gl_Position = perFrameBuffer.Projection * rotationMatrix * worldPosition;;
}
