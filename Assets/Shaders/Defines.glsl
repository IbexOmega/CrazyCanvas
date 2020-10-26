#ifndef DEFINES_SHADER
#define DEFINES_SHADER

#define BUFFER_SET_INDEX 0
#define TEXTURE_SET_INDEX 1
#define DRAW_SET_INDEX 2
#define DRAW_EXTENSION_SET_INDEX 3

#define NO_BUFFERS_TEXTURE_SET_INDEX 0
#define NO_BUFFERS_DRAW_SET_INDEX 1
#define NO_TEXTURES_DRAW_SET_INDEX 1
#define NO_BUFFERS_OR_TEXTURES_DRAW_SET_INDEX 0

#define NO_BUFFERS_DRAW_EXTENSION_SET_INDEX 2
#define NO_TEXTURES_DRAW_EXTENSION_SET_INDEX 2
#define NO_BUFFERS_OR_TEXTURES_DRAW_EXTENSION_SET_INDEX 1

#define UNWRAP_DRAW_SET_INDEX 3

#define MAX_UNIQUE_MATERIALS 32

const float INV_PI			= 1.0f / 3.14159265359f;
const float FOUR_PI			= 12.5663706144f;
const float TWO_PI			= 6.28318530718f;
const float PI 				= 3.14159265359f;
const float PI_OVER_TWO		= 1.57079632679f;
const float PI_OVER_FOUR	= 0.78539816330f;
const float EPSILON			= 0.001f;
const float GAMMA			= 2.2f;

const float MAX_TEMPORAL_FRAMES = 256.0f;

#define MAX_NUM_AREA_LIGHTS 4
#define AREA_LIGHT_TYPE_QUAD 1

#define HIT_MASK_GAME_OBJECT    0x01
#define HIT_MASK_LIGHT          0x02

// Used in PointShadowDepthTest for PCF soft shadows
const vec3 sampleOffsetDirections[20] = vec3[]
(
   vec3( 1,  1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1,  1,  1), 
   vec3( 1,  1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1,  1, -1),
   vec3( 1,  1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1,  1,  0),
   vec3( 1,  0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1,  0, -1),
   vec3( 0,  1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0,  1, -1)
);  

struct SPositions
{
	vec3 WorldPos; 
	vec3 ViewPos;
};

struct SVertex
{
	vec4 Position;
	vec4 Normal;
	vec4 Tangent;
	vec4 TexCoord;
};

struct SMeshlet
{
	uint VertCount;
	uint VertOffset;
	uint PrimCount;
	uint PrimOffset;
};

struct SPrimaryInstance
{
	mat3x4	Transform;
	uint	Mask_IndirectArgIndex;
	uint	SBTRecordOffset_Flags;
	uint	AccelerationStructureHandleTop32;
	uint	AccelerationStructureHandleBottom32;
};

struct SInstance
{
	mat4 Transform;
	mat4 PrevTransform;
	uint MaterialSlot;
	uint ExtensionIndex;
	uint MeshletCount;
	uint Padding0;
};

struct SIndirectArg
{
	uint	IndexCount;
	uint	InstanceCount;
	uint	FirstIndex;
	int		VertexOffset;
	uint	FirstInstance;
	
	uint	MaterialIndex;
};

struct SAreaLight
{
	uint	InstanceIndex;
	uint	Type;
	uvec2	Padding;
};

struct SPointLight
{
    vec4    ColorIntensity;
    vec3    Position;
	float	NearPlane;
	float	FarPlane;
	uint	TextureIndex;
	vec2	padding0;
    mat4    ProjView[6];
};

struct SLightsBuffer
{
    vec4        DirL_ColorIntensity;
	vec3        DirL_Direction;
    float		PointLightCount;
    mat4        DirL_ProjView;
};

struct SPerFrameBuffer
{
	mat4 Projection;
	mat4 View;
	mat4 PrevProjection;
	mat4 PrevView;
	mat4 ViewInv;
	mat4 ProjectionInv;
	vec4 CameraPosition;
	vec4 CameraRight;
	vec4 CameraUp;
	vec2 Jitter;

	uint FrameIndex;
	uint RandomSeed;
};

struct SMaterialParameters
{
	vec4	Albedo;
	float	AO;
	float	Metallic;
	float	Roughness;
	float	Unused;
};

struct SShapeSample
{
	vec3	Position;
	vec3	Normal;
	float	PDF;
};

struct SUnwrapData
{
	vec4 TargetPosition;
	vec4 TargetDirection;
	uint PaintMode;
	uint RemoteMode;
	uint TeamMode;
};

#endif