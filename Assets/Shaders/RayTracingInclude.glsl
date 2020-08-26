#include "Defines.glsl"
#include "EpicBRDF.glsl"

const float RAY_NORMAL_OFFSET   = 0.01f;

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
    vec3    Scatter_f;
    float   Scatter_PDF;
    vec3    SampleWorldDir;
    float   DistanceToSamplePoint;
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

layout(binding = 0, set = BUFFER_SET_INDEX) uniform accelerationStructureEXT                                                u_TLAS;
layout(binding = 1, set = BUFFER_SET_INDEX) restrict readonly buffer Vertices               { SVertex val[]; }              b_Vertices;
layout(binding = 2, set = BUFFER_SET_INDEX) restrict readonly buffer Indices                { uint val[]; }                 b_Indices;
layout(binding = 3, set = BUFFER_SET_INDEX) restrict readonly buffer PrimaryInstances       { SPrimaryInstance val[]; }     b_PrimaryInstances;
layout(binding = 4, set = BUFFER_SET_INDEX) restrict readonly buffer IndirectArgs           { SIndirectArg val[]; }         b_IndirectArgs;
layout(binding = 5, set = BUFFER_SET_INDEX) restrict readonly buffer MaterialParameters     { SMaterialParameters val[]; }  b_MaterialParameters;
layout(binding = 6, set = BUFFER_SET_INDEX) uniform LightsBuffer                            { SLightsBuffer val; }          u_LightsBuffer;
layout(binding = 7, set = BUFFER_SET_INDEX) uniform PerFrameBuffer                          { SPerFrameBuffer val; }        u_PerFrameBuffer;

layout(binding = 0,     set = TEXTURE_SET_INDEX) uniform sampler2D 	                                u_AlbedoAO;
layout(binding = 1,     set = TEXTURE_SET_INDEX) uniform sampler2D 	                                u_CompactNormals;
layout(binding = 2,     set = TEXTURE_SET_INDEX) uniform sampler2D 	                                u_EmissionMetallicRoughness;
layout(binding = 3,     set = TEXTURE_SET_INDEX) uniform sampler2D 	                                u_DepthStencil;
layout(binding = 4,     set = TEXTURE_SET_INDEX) uniform sampler2D                                  u_SceneAlbedoMaps[MAX_UNIQUE_MATERIALS];
layout(binding = 5,     set = TEXTURE_SET_INDEX) uniform sampler2D                                  u_SceneNormalMaps[MAX_UNIQUE_MATERIALS];
layout(binding = 6,     set = TEXTURE_SET_INDEX) uniform sampler2D                                  u_SceneAOMaps[MAX_UNIQUE_MATERIALS];
layout(binding = 7,     set = TEXTURE_SET_INDEX) uniform sampler2D                                  u_SceneMetallicMaps[MAX_UNIQUE_MATERIALS];
layout(binding = 8,     set = TEXTURE_SET_INDEX) uniform sampler2D                                  u_SceneRoughnessMaps[MAX_UNIQUE_MATERIALS];
layout(binding = 9,     set = TEXTURE_SET_INDEX) uniform sampler2DArray                             u_BlueNoiseLUT;
layout(binding = 10,    set = TEXTURE_SET_INDEX, rgba32f) restrict writeonly uniform image2D   		u_DirectRadiance;
layout(binding = 11,    set = TEXTURE_SET_INDEX, rgba32f) restrict writeonly uniform image2D   		u_IndirectRadiance;
layout(binding = 12,    set = TEXTURE_SET_INDEX, rgba32f) restrict writeonly uniform image2D   		u_DirectAlbedo;
layout(binding = 13,    set = TEXTURE_SET_INDEX, rgba32f) restrict writeonly uniform image2D   		u_IndirectAlbedo;


SRayDirections CalculateRayDirections(vec3 hitPosition, vec3 normal, vec3 cameraPosition, mat4 cameraViewInv)
{
	vec4 u_CameraOrigin = cameraViewInv * vec4(0.0f, 0.0f, 0.0f, 1.0f);
	vec3 origDirection = normalize(hitPosition - cameraPosition);

    SRayDirections rayDirections;
	rayDirections.ReflDir = reflect(origDirection, normal);
	rayDirections.ViewDir = -origDirection;
    return rayDirections;
}
RayDirections CalculateRayDirections(vec3 hitPosition, vec3 normal, vec3 cameraPosition, mat4 cameraViewInv)
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
    SIndirectArg indirectArg = b_IndirectArgs.val[indirectArgIndex];
	vec3 barycentricCoords = vec3(1.0f - attribs.x - attribs.y, attribs.x, attribs.y);

    uint materialIndex      = indirectArg.MaterialIndex;
	uint meshVertexOffset   = indirectArg.VertexOffset;
	uint meshIndexOffset    = indirectArg.FirstIndex;

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

	vec3 T = normalize(v0.Tangent.xyz * barycentricCoords.x + v1.Tangent.xyz * barycentricCoords.y + v2.Tangent.xyz * barycentricCoords.z);
	vec3 N  = normalize(v0.Normal.xyz * barycentricCoords.x + v1.Normal.xyz * barycentricCoords.y + v2.Normal.xyz * barycentricCoords.z);

	T = normalize(objectToWorld * vec4(T, 0.0f));
	N = normalize(objectToWorld * vec4(N, 0.0f));
	T = normalize(T - dot(T, N) * N);
	vec3 B = cross(N, T);
	mat3 TBN = mat3(T, B, N);

	vec3 shadingNormal = texture(u_SceneNormalMaps[materialIndex], texCoord).xyz;
	shadingNormal = normalize(shadingNormal * 2.0f - 1.0f);
	shadingNormal = TBN * shadingNormal;

    SRayHitDescription hitDescription;
    hitDescription.ShadingNormal    = shadingNormal;
    hitDescription.GeometricNormal  = N;
    hitDescription.TexCoord         = texCoord;
    hitDescription.MaterialIndex    = materialIndex;

    return hitDescription;
}

SLightSample EvalDirectionalRadiance(vec3 w_o, vec3 albedo, float metallic, float roughness, mat3 worldToLocal)
{
    SLightSample sampleData;
    sampleData.DistanceToSamplePoint    = 0.0f;
    sampleData.SampleWorldDir           = u_LightsBuffer.val.DirL_Direction.xyz;
    sampleData.InstanceIndex            = -1;
    
    vec3 w_i 		= worldToLocal * sampleData.SampleWorldDir;

    //Evaluate the BRDF in the Light Direction (ofcourse we don't need to sample it since we already know w_i)
    SReflection reflection = f(w_o, w_i, albedo, metallic, roughness);

    sampleData.Scatter_f    = reflection.f;
    sampleData.Scatter_PDF  = reflection.PDF;         
    sampleData.L_i          = u_LightsBuffer.val.DirL_EmittedRadiance.rgb;
    sampleData.Light_PDF    = 1.0f;

    return sampleData;
}

SLightSample SampleAreaLightRadiance(vec3 w_o, uint areaLightIndex, vec3 worldPosition, vec3 albedo, float metallic, float roughness, mat3 worldToLocal, vec2 u)
{
    SLightSample sampleData;
    sampleData.DistanceToSamplePoint    = 0.0f;
    sampleData.SampleWorldDir           = vec3(0.0f);
    sampleData.Scatter_f                = vec3(0.0f);
    sampleData.Scatter_PDF              = 0.0f;
    sampleData.L_i                      = vec3(0.0f);
    sampleData.Light_PDF                = 0.0f;
    
    SAreaLight areaLight                = u_LightsBuffer.val.AreaLights[areaLightIndex];
    SPrimaryInstance primaryInstance    = b_PrimaryInstances.val[areaLight.InstanceIndex];
    sampleData.InstanceIndex            = int(areaLight.InstanceIndex);

    mat4 transform;
	transform[0] = vec4(primaryInstance.Transform[0]);
	transform[1] = vec4(primaryInstance.Transform[1]);
	transform[2] = vec4(primaryInstance.Transform[2]);
	transform[3] = vec4(0.0f, 0.0f, 0.0f, 1.0f);
    transform = transpose(transform);

    SShapeSample shapeSample;

    if (areaLight.Type == AREA_LIGHT_TYPE_QUAD)
    {
        shapeSample = SampleQuad(transform, u);
    }
    else
    {
        return sampleData;
    }
    
    vec3 deltaPos = shapeSample.Position - worldPosition;

    float distSqrdToSamplePoint         = dot(deltaPos, deltaPos);
    sampleData.DistanceToSamplePoint    = sqrt(distSqrdToSamplePoint);
    sampleData.SampleWorldDir           = sampleData.DistanceToSamplePoint > 0.0f ? deltaPos / sampleData.DistanceToSamplePoint : -shapeSample.Normal;

    //Check if the disk is facing us
    if (shapeSample.PDF > 0.0f && dot(-deltaPos, shapeSample.Normal) > 0.0f)
    {
        vec3 w_i 		        = worldToLocal * sampleData.SampleWorldDir;

        //Evaluate the BRDF in the Light Direction (ofcourse we don't need to sample it since we already know w_i)
        SReflection reflection                      = f(w_o, w_i, albedo, metallic, roughness);

        uint indirectArgID                          = (primaryInstance.Mask_IndirectArgIndex) & 0x00FFFFFF;
        SIndirectArg indirectArg                    = b_IndirectArgs.val[indirectArgID];
        uint materialIndex                          = indirectArg.MaterialIndex;

        SMaterialParameters materialParameters      = b_MaterialParameters.val[materialIndex];

        sampleData.Scatter_f    = reflection.f;
        sampleData.Scatter_PDF  = reflection.PDF;
        sampleData.L_i          = pow(materialParameters.Albedo.rgb * texture(u_SceneAlbedoMaps[materialIndex], vec2(0.0f)).rgb, vec3(GAMMA)) * materialParameters.EmissionStrength;
        sampleData.Light_PDF    = shapeSample.PDF * distSqrdToSamplePoint / abs(dot(shapeSample.Normal, -sampleData.SampleWorldDir)); //Convert from area measure, as returned by the Sample(Shape)() call above, to solid angle measure.
    }

    return sampleData;
}

SLightEval EvalAreaLightRadience(vec3 w_i_world, float distanceToSamplePoint, uint areaLightIndex)
{
    SAreaLight areaLight                = u_LightsBuffer.val.AreaLights[areaLightIndex];
    SPrimaryInstance primaryInstance    = b_PrimaryInstances.val[areaLight.InstanceIndex];

    mat4 transform;
	transform[0] = vec4(primaryInstance.Transform[0]);
	transform[1] = vec4(primaryInstance.Transform[1]);
	transform[2] = vec4(primaryInstance.Transform[2]);
	transform[3] = vec4(0.0f, 0.0f, 0.0f, 1.0f);
    transform = transpose(transform);

    SLightEval lightEval;
    lightEval.L_i = vec3(0.0f);
    lightEval.PDF = 0.0f;

    vec3 lightNormal = vec3(0.0f);

    if (areaLight.Type == AREA_LIGHT_TYPE_QUAD)
    {
        lightEval.PDF   = QuadPDF(transform);
        lightNormal     = QuadNormal(transform);
    }
    else
    {
        return lightEval;
    }

    uint indirectArgID                          = (primaryInstance.Mask_IndirectArgIndex) & 0x00FFFFFF;
    SIndirectArg indirectArg                    = b_IndirectArgs.val[indirectArgID];
    uint materialIndex                          = indirectArg.MaterialIndex;

    SMaterialParameters materialParameters      = b_MaterialParameters.val[materialIndex];
    
    lightEval.L_i           = pow(materialParameters.Albedo.rgb * texture(u_SceneAlbedoMaps[materialIndex], vec2(0.0f)).rgb, vec3(GAMMA)) * materialParameters.EmissionStrength;
    lightEval.PDF           *= (distanceToSamplePoint * distanceToSamplePoint) / abs(dot(lightNormal, -w_i_world));
    return lightEval;
}

float GenerateSample(uint index, uvec3 p, uint numSamplesPerFrame, uvec3 blueNoiseSize)
{
	p.z = (p.z * numSamplesPerFrame + index);
	p &= (blueNoiseSize - 1);

	return min(texelFetch(u_BlueNoiseLUT, ivec3(p), 0).r, 0.9999999999999f);
}