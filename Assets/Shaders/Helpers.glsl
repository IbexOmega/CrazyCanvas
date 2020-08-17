#ifndef HELPERS_SHADER
#define HELPERS_SHADER

#include "Defines.glsl"

vec3 CalculateNormal(vec4 sampledNormalMetallicRoughness)
{
	vec3 normal;
	normal.xy 	= sampledNormalMetallicRoughness.xy;
	normal.z 	= sqrt(1.0f - dot(normal.xy, normal.xy));
	if (sampledNormalMetallicRoughness.a < 0)
	{
		normal.z = -normal.z;
	}
	normal = normalize(normal);
	return normal;
}

SPositions CalculatePositionsFromDepth(vec2 screenTexCoord, float sampledDepth, mat4 cameraProjectionInv, mat4 cameraViewInv)
{
	vec2 xy = screenTexCoord * 2.0f - 1.0f;

	vec4 clipSpacePosition = vec4(xy, sampledDepth, 1.0f);
	vec4 viewSpacePosition = cameraProjectionInv * clipSpacePosition;
	viewSpacePosition = viewSpacePosition / viewSpacePosition.w;
	vec4 homogenousPosition = cameraViewInv * viewSpacePosition;

    SPositions positions;
	positions.WorldPos  = homogenousPosition.xyz;
	positions.ViewPos   = viewSpacePosition.xyz;
    return positions;
}

float LinearizeDepth(float depth, float near, float far)
{
    float z = depth * 2.0 - 1.0;
    return (2.0 * near * far) / (far + near - z * (far - near));
}

/*
	Schlick Fresnel function
*/
vec3 Fresnel(vec3 F0, float cosTheta)
{
	return F0 + ((vec3(1.0f) - F0) * pow(1.0f - cosTheta, 5.0f));
}

/*
	Schlick Fresnel function with roughness
*/
vec3 FresnelRoughness(vec3 F0, float cosTheta, float roughness)
{
	return F0 + ((max(vec3(1.0f - roughness), F0) - F0) * pow(1.0f - cosTheta, 5.0f));
}

/*
	GGX Distribution function
*/
float Distribution(vec3 normal, vec3 halfVector, float roughness)
{
	float roughnessSqrd = roughness * roughness;
	float alphaSqrd 	= max(roughnessSqrd * roughnessSqrd, 0.0000001f);

	float NdotH = max(dot(normal, halfVector), 0.0f);

	float denom = ((NdotH * NdotH) * (alphaSqrd - 1.0f)) + 1.0f;
	return alphaSqrd / (PI * denom * denom);
}

/*
	Schlick and GGX Geometry-Function
*/
float GeometryGGX(float NdotV, float roughness)
{
	float r 		= roughness + 1.0f;
	float k_Direct 	= (r*r) / 8.0f;

	return NdotV / ((NdotV * (1.0f - k_Direct)) + k_Direct);
}

/*
	Smith Geometry-Function
*/
float Geometry(vec3 normal, vec3 viewDir, vec3 lightDirection, float roughness)
{
	float NdotV = max(dot(normal, viewDir), 0.0f);
	float NdotL = max(dot(normal, lightDirection), 0.0f);

	return GeometryGGX(NdotV, roughness) * GeometryGGX(NdotL, roughness);
}

/*
	Smith Geometry-Function
*/
float GeometryOpt(float NdotV, float NdotL, float roughness)
{
	return GeometryGGX(NdotV, roughness) * GeometryGGX(NdotL, roughness);
}

vec3 ToneMap(vec3 color, float gamma)
{
	vec3 result = color / (color + vec3(1.0f));
	result = pow(result, vec3(1.0f / gamma));
	return result;
}

float GoldNoise(vec3 x, float seed, float min, float max)
{
    const float BASE_SEED   = 10000.0f;
    const float GOLD_PHI    = 1.61803398874989484820459 * 00000.1; // Golden Ratio   
    const float GOLD_PI     = 3.14159265358979323846264 * 00000.1; // PI
    const float GOLD_SQ2    = 1.41421356237309504880169 * 10000.0; // Square Root of Two
    const float GOLD_E      = 2.71828182846;

    return mix(min, max, fract(tan(distance(x * (BASE_SEED + seed + GOLD_PHI), vec3(GOLD_PHI, GOLD_PI, GOLD_E))) * GOLD_SQ2) * 0.5f + 0.5f);
}

void CreateCoordinateSystem(in vec3 N, out vec3 Nt, out vec3 Nb) 
{ 
    if (abs(N.x) > abs(N.y)) 
        Nt = vec3(N.z, 0, -N.x) / sqrt(N.x * N.x + N.z * N.z); 
    else 
        Nt = vec3(0, -N.z, N.y) / sqrt(N.y * N.y + N.z * N.z); 
    Nt = vec3(N.z, 0, -N.x) / sqrt(N.x * N.x + N.z * N.z); 
    Nb = cross(N, Nt); 
} 

vec3 ReflectanceDirection(vec3 reflDir, vec3 Rt, vec3 Rb, float roughness, vec2 uniformRandom)
{
    float specularExponent = 2.0f / (pow(roughness, 4.0f)) - 2.0f;

    if (specularExponent > 2048.0f)
        return reflDir;

    float cosTheta = pow(0.244f, 1.0f / (specularExponent + 1.0f));
    float z = mix(cosTheta, 1.0f, uniformRandom.x);
    float phi = mix(-2.0f * PI, 2.0f * PI, uniformRandom.y);
    float sinTheta = sqrt(1.0f - z * z);

    vec3 coneVector = vec3(sinTheta * cos(phi), z, sinTheta * sin(phi));
    return vec3(
        coneVector.x * Rb.x + coneVector.y * reflDir.x + coneVector.z * Rt.x, 
        coneVector.x * Rb.y + coneVector.y * reflDir.y + coneVector.z * Rt.y, 
        coneVector.x * Rb.z + coneVector.y * reflDir.z + coneVector.z * Rt.z); 
}

vec3 SphericalToDirection(float sinTheta, float cosTheta, float phi)
{
    return vec3(sinTheta * cos(phi), 
                sinTheta * sin(phi), 
                cosTheta);
}

float SameHemisphere(vec3 w_0, vec3 w_1)
{
    return step(0.0f, w_0.z * w_1.z) * 2.0f - 1.0f;
}

float RadicalInverse_VdC(uint bits) 
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

vec2 Hammersley(uint i, uint N)
{
    return vec2(float(i) / float(N), RadicalInverse_VdC(i));
}  

#endif
