#ifndef DEFINES_SHADER
#define DEFINES_SHADER

#define BUFFER_SET_INDEX 0
#define TEXTURE_SET_INDEX 1

#define NO_BUFFERS_TEXTURE_SET_INDEX 0

#define MAX_UNIQUE_MATERIALS 32

const float INV_PI 		    = 1.0f / 3.14159265359f;
const float FOUR_PI         = 12.5663706144f;
const float TWO_PI          = 6.28318530718f;
const float PI 		        = 3.14159265359f;
const float PI_OVER_TWO     = 1.57079632679f;
const float PI_OVER_FOUR    = 0.78539816330f;
const float EPSILON         = 0.001f;
const float GAMMA           = 2.2f;

const float MAX_TEMPORAL_FRAMES = 256.0f;

#define MAX_NUM_AREA_LIGHTS 4
#define AREA_LIGHT_TYPE_QUAD 1

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

struct SPrimaryInstance
{
    mat3x4  Transform;
    uint    Mask_IndirectArgIndex;
    uint    SBTRecordOffset_Flags;
    uint    AccelerationStructureHandleTop32;
    uint    AccelerationStructureHandleBottom32;
};

struct SSecondaryInstance
{
    mat4    PrevTransform;
};

struct SIndirectArg
{
    uint	IndexCount;
    uint	InstanceCount;
    uint	FirstIndex;
    int	    VertexOffset;
    uint	FirstInstance;
    
    uint	MaterialIndex;
};

struct SAreaLight
{
    uint    InstanceIndex;
    uint    Type;
    uvec2   Padding;
};

struct SLightsBuffer
{
    vec4        DirL_Direction;
	vec4        DirL_EmittedRadiance;
    SAreaLight  AreaLights[MAX_NUM_AREA_LIGHTS];
    uint        AreaLightCount;
};

struct SPerFrameBuffer
{
    mat4 Projection;
	mat4 View;
	mat4 PrevProjection;
	mat4 PrevView;
	mat4 ViewInv;
	mat4 ProjectionInv;
	vec4 Position;
	vec4 Right;
	vec4 Up;

    uint FrameIndex;
    uint RandomSeed;
};

struct SMaterialParameters
{
    vec4    Albedo;
    float   Ambient;
    float   Metallic;
    float   Roughness;
    float   EmissionStrength;
};

struct SShapeSample
{
    vec3    Position;
    vec3    Normal;
    float   PDF;
};

#endif