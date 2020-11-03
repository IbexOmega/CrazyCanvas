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

vec3 PackNormal(vec3 normal)
{
	return (normalize(normal) + 1.0f) * 0.5f;
}

vec3 UnpackNormal(vec3 normal)
{
	return normalize((normal * 2.0f) - 1.0f);
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

vec3 GammaCorrection(vec3 color, float gamma)
{
	vec3 result = color / (color + vec3(1.0f));
	result = pow(result, vec3(1.0f / gamma));
	return result;
}

void CreateCoordinateSystem(in vec3 N, out vec3 Nt, out vec3 Nb)
{
	if (abs(N.x) > abs(N.y))  	Nt = vec3(N.z, 0, -N.x) / sqrt(N.x * N.x + N.z * N.z);
	else 						Nt = vec3(0, -N.z, N.y) / sqrt(N.y * N.y + N.z * N.z);
	//Nt = vec3(N.z, 0, -N.x) / sqrt(N.x * N.x + N.z * N.z);
	Nb = cross(N, Nt);
}

vec3 SphericalToDirection(float sinTheta, float cosTheta, float phi)
{
	return vec3(sinTheta * cos(phi),
				sinTheta * sin(phi),
				cosTheta);
}

vec3 SphericalToDirection(float sinTheta, float cosTheta, float phi, vec3 x, vec3 y, vec3 z)
{
	return 	sinTheta * cos(phi) * x +
			sinTheta * sin(phi) * y +
			cosTheta * z;
}

float FlipIfNotSameHemisphere(vec3 w_0, vec3 w_1)
{
	return step(0.0f, w_0.z * w_1.z) * 2.0f - 1.0f;
}

bool IsSameHemisphere(vec3 w_0, vec3 w_1)
{
	return w_0.z * w_1.z > 0.0f;
}



vec2 DirToOct(vec3 normal)
{
	vec2 p = normal.xy * (1.0f / dot(abs(normal), vec3(1.0f)));
	vec2 e = normal.z > 0.0f ? p : (1.0f - abs(p.yx)) * (step(0.0f, p) * 2.0f - vec2(1.0f));
	return e;
}

vec3 OctToDir(vec2 e)
{
	vec3 v = vec3(e, 1.0f - abs(e.x) - abs(e.y));
	if (v.z < 0.0f) v.xy = (1.0f - abs(v.yx)) * (step(0.0f, v.xy) * 2.0f - vec2(1.0f));
	return normalize(v);
}

float PowerHeuristic(float nf, float fPDF, float ng, float gPDF)
{
	float f = nf * fPDF;
	float fSqrd = f * f;
	float g = ng * gPDF;

	return (fSqrd) / (fSqrd + g * g);
}

//Power Heuristic but implicitly divides with fPDF to avoid floating point errors
float PowerHeuristicWithPDF(float nf, float fPDF, float ng, float gPDF)
{
	float f = nf * fPDF;
	float fSqrd = f * f;
	float g = ng * gPDF;

	return (nf * f) / (fSqrd + g * g);
}

#define ONE_NINTH 0.1111111
float DirShadowDepthTest(vec4 fragPosLightSpace, vec3 fragNormal, vec3 lightDir, sampler2D shadowMap)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
	projCoords.xy = (projCoords.xy * 0.5 + 0.5);
	projCoords.y = 1.0 - projCoords.y;

	if (projCoords.z > 1.0)
	{
		return 0.0f;
	}

	float closestDepth = texture(shadowMap, projCoords.xy).r;
	float currentDepth = projCoords.z;

	float bias = max(0.001 * (1.0 - dot(fragNormal, -lightDir)), 0.004);
	float shadow = 0.0;
    
	vec2 texelSize = 1.0 / textureSize(shadowMap, 0); // Todo: send in shadowMap width as pushback constant
	for(int x = -1; x <= 1; ++x)
	{
		for(int y = -1; y <= 1; ++y)
		{
			float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r; 
			shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;        
		}    
	}
	
	shadow *= ONE_NINTH;

    return shadow;
}

float PointShadowDepthTest(vec3 fragPos, vec3 lightPos, float viewDistance, vec3 normal, samplerCube shadowMap, float farPlane)
{
    vec3 fragToLight  = fragPos - lightPos;
	float currentDepth = length(fragToLight);
	vec3 lightDir = fragToLight / currentDepth;

	float shadow		= 0.0f;
	float bias			= max(0.05f * (1.0f - dot(normal, lightDir)), 0.005f);
	float diskRadius	= (1.0f + (viewDistance / farPlane)) / 75.0f;
	int samples			= 20;

	for(int i = 0; i < samples; ++i)
	{
		float closestDepth = texture(shadowMap, fragToLight + sampleOffsetDirections[i] * diskRadius).r;
		closestDepth *= farPlane;   
		if(currentDepth - bias > closestDepth)
			shadow += 1.0;
	}
	shadow /= float(samples);  

    return shadow;
}

float CalculateLuminance(vec3 color)
{
	return sqrt(dot(color, vec3(0.299f, 0.587f, 0.114f)));
}

#endif
