#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shader_draw_parameters : enable
#extension GL_GOOGLE_include_directive : enable

#include "..\Defines.glsl"

struct SPointLightsBuffer
{
    mat4 ProjView[6];
};

layout( push_constant ) uniform PushConstants {
    uint Iteration;
    float Near;
    float Far;
} pc;

layout(binding = 0, set = BUFFER_SET_INDEX) restrict readonly buffer Vertices               { SVertex val[]; }              b_Vertices;
layout(binding = 1, set = BUFFER_SET_INDEX) restrict readonly buffer PrimaryInstances       { SPrimaryInstance val[]; }     b_PrimaryInstances;
layout(binding = 2, set = BUFFER_SET_INDEX) uniform PointLightBuffer                        { SPointLightsBuffer val; }     u_PointLightsBuffer;

layout(location = 0) out vec4 out_WorldPos;

void main()
{
    SPrimaryInstance primaryInstance            = b_PrimaryInstances.val[gl_InstanceIndex];
    SVertex vertex                              = b_Vertices.val[gl_VertexIndex];
    SPointLightsBuffer pointLightsBuffer        = u_PointLightsBuffer.val;

    //Calculate a 4x4 matrix so that we calculate the correct w for worldPosition
    mat4 transform;
	transform[0] = vec4(primaryInstance.Transform[0]);
	transform[1] = vec4(primaryInstance.Transform[1]);
	transform[2] = vec4(primaryInstance.Transform[2]);
	transform[3] = vec4(0.0f, 0.0f, 0.0f, 1.0f);
    transform = transpose(transform);

    out_WorldPos = transform * vec4(vertex.Position.xyz, 1.0f);
    gl_Position = pointLightsBuffer.ProjView[pc.Iteration] * out_WorldPos;
}
