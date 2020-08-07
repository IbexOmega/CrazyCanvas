#version 460
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_ray_tracing : enable
//#extension GL_EXT_debug_printf : enable

#include "Helpers.glsl"
#include "Defines.glsl"

struct SRadiancePayload
{
	vec3 	OutgoingRadiance;
};

layout(binding = 1, set = BUFFER_SET_INDEX) buffer Vertices            { SVertex val[]; }              b_Vertices;
layout(binding = 2, set = BUFFER_SET_INDEX) buffer Indices             { uint val[]; }                 b_Indices;
layout(binding = 3, set = BUFFER_SET_INDEX) buffer Instances           { SInstance val[]; }            b_Instances;
layout(binding = 4, set = BUFFER_SET_INDEX) buffer MeshIndices         { SMeshIndexDesc val[]; }       b_MeshIndices;
layout(binding = 5, set = BUFFER_SET_INDEX) buffer MaterialParameters  { SMaterialParameters val[]; }  b_MaterialParameters;

layout(binding = 6, set = BUFFER_SET_INDEX) uniform LightsBuffer       { SLightsBuffer val; }          u_LightsBuffer;
layout(binding = 7, set = BUFFER_SET_INDEX) uniform PerFrameBuffer     { SPerFrameBuffer val; }        u_PerFrameBuffer;

layout(binding = 3, set = TEXTURE_SET_INDEX) uniform sampler2D u_SceneAlbedoMaps[MAX_UNIQUE_MATERIALS];
layout(binding = 4, set = TEXTURE_SET_INDEX) uniform sampler2D u_SceneNormalMaps[MAX_UNIQUE_MATERIALS];
layout(binding = 5, set = TEXTURE_SET_INDEX) uniform sampler2D u_SceneAOMaps[MAX_UNIQUE_MATERIALS];
layout(binding = 6, set = TEXTURE_SET_INDEX) uniform sampler2D u_SceneMetallicMaps[MAX_UNIQUE_MATERIALS];
layout(binding = 7, set = TEXTURE_SET_INDEX) uniform sampler2D u_SceneRoughnessMaps[MAX_UNIQUE_MATERIALS];

layout(location = 0) rayPayloadInEXT SRadiancePayload s_RadiancePayload;

hitAttributeEXT vec3 attribs;

SRayHitDescription CalculateTriangleData()
{
    SMeshIndexDesc meshIndexDesc = b_MeshIndices.val[gl_InstanceCustomIndexEXT];
	vec3 barycentricCoords = vec3(1.0f - attribs.x - attribs.y, attribs.x, attribs.y);

    uint materialIndex      = meshIndexDesc.MaterialIndex;
	uint meshVertexOffset   = meshIndexDesc.VertexOffset;
	uint meshIndexOffset    = meshIndexDesc.FirstIndex;

	ivec3 index = ivec3
    (
        b_Indices.val[meshIndexOffset + 3 * gl_PrimitiveID], 
        b_Indices.val[meshIndexOffset + 3 * gl_PrimitiveID + 1], 
        b_Indices.val[meshIndexOffset + 3 * gl_PrimitiveID + 2]
    );

	SVertex v0 = b_Vertices.val[meshVertexOffset + index.x];
	SVertex v1 = b_Vertices.val[meshVertexOffset + index.y];
	SVertex v2 = b_Vertices.val[meshVertexOffset + index.z];

	vec2 texCoord = (v0.TexCoord.xy * barycentricCoords.x + v1.TexCoord.xy * barycentricCoords.y + v2.TexCoord.xy * barycentricCoords.z);

	mat4 transform;
	transform[0] = vec4(gl_ObjectToWorldEXT[0], 0.0f);
	transform[1] = vec4(gl_ObjectToWorldEXT[1], 0.0f);
	transform[2] = vec4(gl_ObjectToWorldEXT[2], 0.0f);
	transform[3] = vec4(gl_ObjectToWorldEXT[3], 1.0f);

	vec3 T = normalize(v0.Tangent.xyz * barycentricCoords.x + v1.Tangent.xyz * barycentricCoords.y + v2.Tangent.xyz * barycentricCoords.z);
	vec3 N  = normalize(v0.Normal.xyz * barycentricCoords.x + v1.Normal.xyz * barycentricCoords.y + v2.Normal.xyz * barycentricCoords.z);

	T = normalize(vec3(transform * vec4(T, 0.0)));
	N = normalize(vec3(transform * vec4(N, 0.0)));
	T = normalize(T - dot(T, N) * N);
	vec3 B = cross(N, T);
	mat3 TBN = mat3(T, B, N);

	vec3 normal = texture(u_SceneNormalMaps[materialIndex], texCoord).xyz;
	normal = normalize(normal * 2.0f - 1.0f);
	normal = TBN * normal;

    SRayHitDescription hitDescription;
    hitDescription.Normal           = normal;
    hitDescription.TexCoord         = texCoord;
    hitDescription.MaterialIndex    = materialIndex;
	hitDescription.LocalToWorld		= TBN;

    return hitDescription;
}

void main() 
{
    SRayHitDescription hitDescription = CalculateTriangleData();

    vec3 albedo = pow(  texture(u_SceneAlbedoMaps[hitDescription.MaterialIndex],    hitDescription.TexCoord).rgb, vec3(GAMMA));

    s_RadiancePayload.OutgoingRadiance = albedo;
}