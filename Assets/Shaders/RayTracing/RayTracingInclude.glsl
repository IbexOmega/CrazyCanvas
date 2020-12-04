#extension GL_EXT_nonuniform_qualifier : enable

#include "../Defines.glsl"
#include "../Helpers.glsl"

const float RAY_NORMAL_OFFSET   = 0.1f;

struct SPrimaryPayload
{
	vec3    HitPosition;
	vec3    Normal;
	vec3    Albedo;
	float 	AO;
	float   Roughness;
	float   Metallic;
	float   Distance;
};

struct SRayDirections
{
	vec3    ReflDir; 
	vec3    ViewDir;
};

layout(binding = 0, set = BUFFER_SET_INDEX) uniform accelerationStructureEXT u_TLAS;

layout(binding = 1, set = BUFFER_SET_INDEX) uniform PerFrameBuffer
{ 
	SPerFrameBuffer val; 
} u_PerFrameBuffer;

layout(binding = 2, set = BUFFER_SET_INDEX) restrict readonly buffer LightsBuffer	
{
	SLightsBuffer val; 
	SPointLight pointLights[];
} b_LightsBuffer;

layout(binding = 3,		set = BUFFER_SET_INDEX) readonly buffer MaterialParameters	{ SMaterialParameters val[]; }	u_MaterialParameters;
layout(binding = 4, 	set = BUFFER_SET_INDEX) readonly buffer PaintMaskColors		{ vec4 val[]; }					b_PaintMaskColor;

layout(binding = 5,		set = BUFFER_SET_INDEX) readonly buffer ParticleInstances 		{ SParticle Val[]; }			b_ParticleInstances;
layout(binding = 6,		set = BUFFER_SET_INDEX) readonly buffer EmitterInstances		{ SEmitter Val[]; } 			b_EmitterInstances;
layout(binding = 7,		set = BUFFER_SET_INDEX) readonly buffer ParticleIndirectIndices	{ SParticleIndexData Val[]; }	b_ParticleIndirectIndices;
layout(binding = 8,		set = BUFFER_SET_INDEX) readonly buffer Atlases					{ SAtlasData Val[]; } 			b_Atlases;

layout(binding = 0,		set = TEXTURE_SET_INDEX) uniform sampler2D		u_AlbedoMaps[];
layout(binding = 1,		set = TEXTURE_SET_INDEX) uniform sampler2D		u_NormalMaps[];
layout(binding = 2,		set = TEXTURE_SET_INDEX) uniform sampler2D		u_CombinedMaterialMaps[];
layout(binding = 3,		set = TEXTURE_SET_INDEX) uniform samplerCube	u_Skybox;

layout(binding = 4,		set = TEXTURE_SET_INDEX) uniform sampler2D		u_GBufferAlbedo;
layout(binding = 5,		set = TEXTURE_SET_INDEX) uniform sampler2D		u_GBufferAORoughMetalValid;
layout(binding = 6,		set = TEXTURE_SET_INDEX) uniform sampler2D		u_GBufferCompactNormal;
layout(binding = 7,		set = TEXTURE_SET_INDEX) uniform sampler2D		u_GBufferDepthStencil;
layout(binding = 8,		set = TEXTURE_SET_INDEX) uniform sampler2D		u_PaintMaskTextures[];
layout(binding = 9,		set = TEXTURE_SET_INDEX) uniform sampler2D 		u_DirLShadowMap;
layout(binding = 10,	set = TEXTURE_SET_INDEX) uniform samplerCube 	u_PointLShadowMap[];
layout(binding = 11,	set = TEXTURE_SET_INDEX) uniform sampler2D 		u_TextureAtlases[];
layout(binding = 12,	set = TEXTURE_SET_INDEX) uniform samplerCube 	u_GlobalSpecularProbe;
layout(binding = 13,	set = TEXTURE_SET_INDEX) uniform samplerCube 	u_GlobalDiffuseProbe;
layout(binding = 14,	set = TEXTURE_SET_INDEX) uniform sampler2D 		u_IntegrationLUT;
layout(binding = 15,	set = TEXTURE_SET_INDEX) uniform sampler2DArray u_BlueNoiseLUTs;
layout(binding = 16,	set = TEXTURE_SET_INDEX, rgba8) restrict uniform image2D	u_Reflections;
layout(binding = 17,	set = TEXTURE_SET_INDEX, rgba16f) restrict uniform image2D	u_Specular_BRDF_PDF;
layout(binding = 18,	set = TEXTURE_SET_INDEX, rgba16f) restrict uniform image2D	u_BRDF_PDF;

#include "../MeshPaintHelper.glsl"