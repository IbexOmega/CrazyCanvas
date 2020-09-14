#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shader_draw_parameters : enable
#extension GL_GOOGLE_include_directive : enable

#include "..\Defines.glsl"

layout (constant_id = 0) const uint NUM_POINT_LIGHTS                  = 2;

struct SPointLight
{
    vec4 Position;
    mat4 ProjViews[6];
};

layout( push_constant ) uniform PushConstants {
    uint Iteration;
    float Near;
    float Far;
} pc;

layout(binding = 0, set = BUFFER_SET_INDEX) restrict readonly buffer Vertices               { SVertex val[]; }                      b_Vertices;
layout(binding = 1, set = BUFFER_SET_INDEX) restrict readonly buffer PrimaryInstances       { SPrimaryInstance val[]; }             b_PrimaryInstances;
layout(binding = 2, set = BUFFER_SET_INDEX) uniform PointLightBuffer                        { SPointLight val[NUM_POINT_LIGHTS]; }  u_PointLightsBuffer;

layout(location = 0) out vec4 out_WorldPos;
layout(location = 1) out flat vec4 out_PointLightPosition;

void main()
{
    uint pointLightIndex    = pc.Iteration / 6;
    uint faceIndex          = pc.Iteration % 6;

    SPrimaryInstance primaryInstance            = b_PrimaryInstances.val[gl_InstanceIndex];
    SVertex vertex                              = b_Vertices.val[gl_VertexIndex];
    SPointLight pointLight                      = u_PointLightsBuffer.val[pointLightIndex];

    //Calculate a 4x4 matrix so that we calculate the correct w for worldPosition
    mat4 transform;
	transform[0] = vec4(primaryInstance.Transform[0]);
	transform[1] = vec4(primaryInstance.Transform[1]);
	transform[2] = vec4(primaryInstance.Transform[2]);
	transform[3] = vec4(0.0f, 0.0f, 0.0f, 1.0f);
    transform = transpose(transform);

    out_WorldPos            = transform * vec4(vertex.Position.xyz, 1.0f);
    out_PointLightPosition  = pointLight.Position;
    gl_Position = pointLight.ProjViews[faceIndex] * out_WorldPos;
}
