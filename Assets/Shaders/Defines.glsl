#ifndef DEFINES_SHADER
#define DEFINES_SHADER

#define BUFFER_SET_INDEX 0
#define TEXTURE_SET_INDEX 1

#define NO_BUFFERS_TEXTURE_SET_INDEX 0

#define MAX_UNIQUE_MATERIALS 32

const float PI 		= 3.14159265359f;
const float TWO_PI  = 6.28318530718f;
const float EPSILON = 0.001f;
const float GAMMA   = 2.2f;

const float MAX_TEMPORAL_FRAMES = 256.0f;

struct SPositions
{
    vec3 WorldPos; 
    vec3 ViewPos;
};

struct SVertex
{
    vec4    Position;
    vec4    Normal;
    vec4    Tangent;
    vec4    TexCoord;
};

struct SInstance
{
    mat3x4  Transform;
    uint    Mask_MeshMaterialIndex;
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
};

struct SLightsBuffer
{
    vec4    Direction;
	vec4    EmittedRadiance;
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

    float FrameIndex;
};

struct SMaterialParameters
{
    vec4    Albedo;
    float   Ambient;
    float   Metallic;
    float   Roughness;
    float   Unreserved;
};

#endif