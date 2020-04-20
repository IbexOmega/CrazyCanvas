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

struct SMeshIndexDesc
{
    uint	IndexCount;
    uint	InstanceCount;
    uint	FirstIndex;
    int	    VertexOffset;
    uint	FirstInstance;
    
    uint	MaterialIndex;
    uint	Padding0;
    uint    Padding1;
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
layout(binding = 1, set = 0) buffer Indices             { uint val[]; }                 b_Indices;
layout(binding = 2, set = 0) buffer Instances           { SInstance val[]; }            b_Instances;
layout(binding = 3, set = 0) buffer MeshIndices         { SMeshIndexDesc val[]; }       b_MeshIndices;
layout(binding = 4, set = 0) uniform PerFrameBuffer     { SPerFrameBuffer val; }        u_PerFrameBuffer;

layout(location = 0) out vec3 out_Normal;
layout(location = 1) out vec3 out_Tangent;
layout(location = 2) out vec3 out_Bitangent;
layout(location = 3) out vec2 out_TexCoord;
layout(location = 4) out vec4 out_Position;
layout(location = 5) out flat uint out_MaterialIndex;

void main()
{
    SMeshIndexDesc meshIndicesData  = b_MeshIndices.val[gl_DrawIDARB];
    SInstance instanceData          = b_Instances.val[gl_InstanceIndex];
    SVertex vertex                  = b_Vertices.val[gl_VertexIndex];
    SPerFrameBuffer perFrameBuffer  = u_PerFrameBuffer.val;

    mat4 transform;
	// transform[0] = vec4(instanceData.Transform[0], 0.0f);
	// transform[1] = vec4(instanceData.Transform[1], 0.0f);
	// transform[2] = vec4(instanceData.Transform[2], 0.0f);
	// transform[3] = vec4(instanceData.Transform[3], 1.0f);

    transform[0] = vec4(1.0000, 0.0000, 0.0000, 2.0000);
    transform[1] = vec4(0.0000, 1.0000, 0.0000, 0.0000);
    transform[2] = vec4(0.0000, 0.0000, 1.0000, 0.0000);
    transform[3] = vec4(0.0000, 0.0000, 0.0000, 1.0000);

    vec4 worldPosition  =   instanceData.Transform * vec4(vertex.Position.xyz,  1.0f);
    vec4 worldNormal    =   instanceData.Transform * vec4(vertex.Normal.xyz,    0.0f);

    gl_Position = perFrameBuffer.Projection * perFrameBuffer.View * worldPosition;

    out_Normal 			= worldNormal.xyz;
	out_Tangent 		= vec3(1.0f);
	out_Bitangent 		= vec3(1.0f);
	out_TexCoord 		= vertex.TexCoord.xy;
	out_Position		= vertex.Position;
    out_MaterialIndex   = meshIndicesData.MaterialIndex;
}