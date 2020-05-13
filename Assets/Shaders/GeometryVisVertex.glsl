#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shader_draw_parameters : enable

struct SVertex
{
    vec4    Position;
    vec4    Normal;
    vec4    Tangent;
    vec4    TexCoord;
};

struct SInstance
{
    mat4    Transform;
    uint    MeshMaterialIndex_Mask;
    uint    SBTRecordOffset_Flags;
    uint    AccelerationStructureHandleTop32;
    uint    AccelerationStructureHandleBottom32;
};

struct SPerFrameBuffer
{
    mat4 Projection;
	mat4 View;
	mat4 LastProjection;
	mat4 LastView;
	mat4 ViewInv;
	mat4 ProjectionInv;
	vec4 Position;
	vec4 Right;
	vec4 Up;
};

layout(binding = 0, set = 0) buffer Vertices            { SVertex val[]; }              b_Vertices;
layout(binding = 1, set = 0) buffer Instances           { SInstance val[]; }            b_Instances;
layout(binding = 2, set = 0) uniform PerFrameBuffer     { SPerFrameBuffer val; }        u_PerFrameBuffer;

layout(location = 0) out flat uint out_InstanceID;

void main()
{
    SInstance instance              = b_Instances.val[gl_InstanceIndex];
    SVertex vertex                  = b_Vertices.val[gl_VertexIndex];
    SPerFrameBuffer perFrameBuffer  = u_PerFrameBuffer.val;

    //mat4 transform;
	// transform[0] = vec4(instanceData.Transform[0], 0.0f);
	// transform[1] = vec4(instanceData.Transform[1], 0.0f);
	// transform[2] = vec4(instanceData.Transform[2], 0.0f);
	// transform[3] = vec4(instanceData.Transform[3], 1.0f);

    vec4 worldPosition = instance.Transform * vec4(vertex.Position.xyz,  1.0f);

    gl_Position = perFrameBuffer.Projection * perFrameBuffer.View * worldPosition;
    out_InstanceID = gl_InstanceIndex;
}