#extension GL_EXT_nonuniform_qualifier : enable

#include "../Defines.glsl"

const float RAY_NORMAL_OFFSET   = 0.01f;

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

layout(binding = 0,     set = BUFFER_SET_INDEX) uniform accelerationStructureEXT                                                u_TLAS;
layout(binding = 1,     set = BUFFER_SET_INDEX) uniform PerFrameBuffer      { SPerFrameBuffer val; }                            u_PerFrameBuffer;
layout(binding = 2,     set = BUFFER_SET_INDEX) uniform MaterialParameters  { SMaterialParameters val[MAX_UNIQUE_MATERIALS]; }  u_MaterialParameters;

layout(binding = 0,     set = TEXTURE_SET_INDEX) uniform sampler2D                  u_AlbedoMaps[];
layout(binding = 1,     set = TEXTURE_SET_INDEX) uniform sampler2D                  u_NormalMaps[];
layout(binding = 2,     set = TEXTURE_SET_INDEX) uniform sampler2D                  u_CombinedMaterialMaps[];
layout(binding = 3,     set = TEXTURE_SET_INDEX) uniform samplerCube                u_Skybox;

layout(binding = 4,     set = TEXTURE_SET_INDEX, rgba32f) restrict uniform image2D  u_GBufferPosition;
layout(binding = 5,     set = TEXTURE_SET_INDEX, rgba8) restrict uniform image2D    u_GBufferAlbedo;
layout(binding = 6,     set = TEXTURE_SET_INDEX, rgba8) restrict uniform image2D    u_GBufferAORoughMetalValid;
layout(binding = 7,     set = TEXTURE_SET_INDEX, rgba16f) restrict uniform image2D    u_GBufferCompactNormal;
layout(binding = 8,    set = TEXTURE_SET_INDEX) uniform sampler2D 	                u_GBufferDepthStencil;

SRayDirections CalculateRayDirections(vec3 hitPosition, vec3 normal, vec3 cameraPosition)
{
	vec3 origDirection = normalize(hitPosition - cameraPosition);

    SRayDirections rayDirections;
	rayDirections.ReflDir = reflect(origDirection, normal);
	rayDirections.ViewDir = -origDirection;
    return rayDirections;
}