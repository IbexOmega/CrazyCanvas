#ifndef DEFINES_SHADER
#define DEFINES_SHADER

#define BUFFER_SET_INDEX 0
#define TEXTURE_SET_INDEX 1

#define MAX_UNIQUE_MATERIALS 32

const float PI 		= 3.14159265359f;
const float EPSILON = 0.001f;
const float GAMMA   = 2.2f;

const float MAX_TEMPORAL_FRAMES = 256.0f;

struct SPositions
{
    vec3 WorldPos; 
    vec3 ViewPos;
};

struct SRayDirections
{
    vec3 ReflDir; 
    vec3 ViewDir;
};

struct SRayHitDescription
{
    vec3 Normal;
    vec2 TexCoord;
    uint MaterialIndex;
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

struct SMaterialParameters
{
    vec4    Albedo;
    float   Ambient;
    float   Metallic;
    float   Roughness;
    float   Unreserved;
};

#endif