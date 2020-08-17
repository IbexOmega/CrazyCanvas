#include "Defines.glsl"
#include "EpicBRDF.glsl"

const float RAY_NORMAL_OFFSET   = 0.025;

struct SRayDirections
{
    vec3    ReflDir; 
    vec3    ViewDir;
};

struct SRayHitDescription
{
    vec3    Normal;
    vec2    TexCoord;
    uint    MaterialIndex;
};

struct SLightSample
{
    vec3    L_d;
    float   PDF;
    vec3    SampleWorldDir;
};

struct SRadiancePayload
{
    vec3    ScatterPosition;
    vec3    Albedo;
    float   Metallic;
    float   Roughness;
    bool    Emissive;
    float   Distance;
	mat3    LocalToWorld;
};

struct SShadowPayload
{
	float   Distance;
};

layout(binding = 0, set = BUFFER_SET_INDEX) uniform accelerationStructureEXT   u_TLAS;
layout(binding = 1, set = BUFFER_SET_INDEX) buffer Vertices            { SVertex val[]; }              b_Vertices;
layout(binding = 2, set = BUFFER_SET_INDEX) buffer Indices             { uint val[]; }                 b_Indices;
layout(binding = 3, set = BUFFER_SET_INDEX) buffer Instances           { SInstance val[]; }            b_Instances;
layout(binding = 4, set = BUFFER_SET_INDEX) buffer MeshIndices         { SMeshIndexDesc val[]; }       b_MeshIndices;
layout(binding = 5, set = BUFFER_SET_INDEX) buffer MaterialParameters  { SMaterialParameters val[]; }  b_MaterialParameters;
layout(binding = 6, set = BUFFER_SET_INDEX) uniform LightsBuffer       { SLightsBuffer val; }          u_LightsBuffer;
layout(binding = 7, set = BUFFER_SET_INDEX) uniform PerFrameBuffer     { SPerFrameBuffer val; }        u_PerFrameBuffer;

layout(binding = 0, set = TEXTURE_SET_INDEX) uniform sampler2D 	                u_AlbedoAO;
layout(binding = 1, set = TEXTURE_SET_INDEX) uniform sampler2D 	                u_NormalMetallicRoughness;
layout(binding = 2, set = TEXTURE_SET_INDEX) uniform sampler2D 	                u_DepthStencil;
layout(binding = 3, set = TEXTURE_SET_INDEX) uniform sampler2D                  u_SceneAlbedoMaps[MAX_UNIQUE_MATERIALS];
layout(binding = 4, set = TEXTURE_SET_INDEX) uniform sampler2D                  u_SceneNormalMaps[MAX_UNIQUE_MATERIALS];
layout(binding = 5, set = TEXTURE_SET_INDEX) uniform sampler2D                  u_SceneAOMaps[MAX_UNIQUE_MATERIALS];
layout(binding = 6, set = TEXTURE_SET_INDEX) uniform sampler2D                  u_SceneMetallicMaps[MAX_UNIQUE_MATERIALS];
layout(binding = 7, set = TEXTURE_SET_INDEX) uniform sampler2D                  u_SceneRoughnessMaps[MAX_UNIQUE_MATERIALS];
layout(binding = 8, set = TEXTURE_SET_INDEX) uniform sampler2D 	                u_BlueNoiseLUT[8];
layout(binding = 9, set = TEXTURE_SET_INDEX, rgba16f) uniform image2D   		u_Radiance;


SRayDirections CalculateRayDirections(vec3 hitPosition, vec3 normal, vec3 cameraPosition, mat4 cameraViewInv)
{
	vec4 u_CameraOrigin = cameraViewInv * vec4(0.0f, 0.0f, 0.0f, 1.0f);
	vec3 origDirection = normalize(hitPosition - cameraPosition);

    SRayDirections rayDirections;
	rayDirections.ReflDir = reflect(origDirection, normal);
	rayDirections.ViewDir = -origDirection;
    return rayDirections;
}

SRayHitDescription CalculateHitData(vec3 attribs, int indirectArgIndex, int primitiveID, mat4x3 objectToWorld)
{
    SMeshIndexDesc meshIndexDesc = b_MeshIndices.val[indirectArgIndex];
	vec3 barycentricCoords = vec3(1.0f - attribs.x - attribs.y, attribs.x, attribs.y);

    uint materialIndex      = meshIndexDesc.MaterialIndex;
	uint meshVertexOffset   = meshIndexDesc.VertexOffset;
	uint meshIndexOffset    = meshIndexDesc.FirstIndex;

	ivec3 index = ivec3
    (
        b_Indices.val[meshIndexOffset + 3 * primitiveID], 
        b_Indices.val[meshIndexOffset + 3 * primitiveID + 1], 
        b_Indices.val[meshIndexOffset + 3 * primitiveID + 2]
    );

	SVertex v0 = b_Vertices.val[meshVertexOffset + index.x];
	SVertex v1 = b_Vertices.val[meshVertexOffset + index.y];
	SVertex v2 = b_Vertices.val[meshVertexOffset + index.z];

	vec2 texCoord = (v0.TexCoord.xy * barycentricCoords.x + v1.TexCoord.xy * barycentricCoords.y + v2.TexCoord.xy * barycentricCoords.z);

	mat4 transform;
	transform[0] = vec4(objectToWorld[0], 0.0f);
	transform[1] = vec4(objectToWorld[1], 0.0f);
	transform[2] = vec4(objectToWorld[2], 0.0f);
	transform[3] = vec4(objectToWorld[3], 1.0f);

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

    return hitDescription;
}

SLightSample EvalDirectionalRadiance(vec3 w_o, vec3 albedo, float metallic, float roughness, mat3 worldToLocal)
{
    //Directional Light
    SLightSample sampleData;
    sampleData.L_d 	          = vec3(0.0f);
    sampleData.PDF            = 0.0f;
    sampleData.SampleWorldDir = u_LightsBuffer.val.Direction.xyz;

    vec3 w_i 		= worldToLocal * sampleData.SampleWorldDir;

    vec3 SumL_d     = vec3(0.0f); 	//Describes the reflected radiance sum from light samples taken on this light
    float N 	    = 0.0f;			//Describes the amount of samples taken on this light, L_d should be divided with N before adding it to L_o

    //Evaluate the BRDF in the Light Direction (ofcourse we don't need to sample it since we already know w_i)
    SReflection reflection = f(w_o, w_i, albedo, metallic, roughness);

    if (reflection.PDF > 0.0f)
    {
        SumL_d 	+= reflection.f * u_LightsBuffer.val.EmittedRadiance.rgb; // Since directional lights are described by a delta distribution we do not divide by the PDF (it will be 1) or multiply by the CosWeight

        /*
            If this was light was not described by a delta distribution we would include a PDF as divisor above and we would below !!Sample!! the BRDF with MIS to weight the
        */
    }

    //No matter if the PDF == 0 or if the light is occluded from the pixel this still counts as a sample (obviously)
    N += 1.0f; 

    sampleData.L_d    = SumL_d / N;
    sampleData.PDF    = reflection.PDF;

    return sampleData;
}