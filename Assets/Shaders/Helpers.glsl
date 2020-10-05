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

vec3 GammaCorrection(vec3 color, float gamma)
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
	if (abs(N.x) > abs(N.y))  	Nt = vec3(N.z, 0, -N.x) / sqrt(N.x * N.x + N.z * N.z);
	else 						Nt = vec3(0, -N.z, N.y) / sqrt(N.y * N.y + N.z * N.z);
	//Nt = vec3(N.z, 0, -N.x) / sqrt(N.x * N.x + N.z * N.z);
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

// A utility to convert a vec3 to a 2-component octohedral representation packed into one uint
uint dirToOct(vec3 normal)
{
	vec2 p = normal.xy * (1.0f / dot(abs(normal), vec3(1.0f)));
	vec2 e = normal.z > 0.0f ? p : (1.0f - abs(p.yx)) * (step(0.0f, p) * 2.0f - vec2(1.0f));
	return packSnorm2x16(e.xy);
	//return (asuint(f32tof16(e.y)) << 16) + (asuint(f32tof16(e.x)));
}

vec3 octToDir(uint octo)
{
	vec2 e = unpackSnorm2x16(octo) ;
	vec3 v = vec3(e, 1.0f - abs(e.x) - abs(e.y));
	if (v.z < 0.0f)
		v.xy = (1.0f - abs(v.yx)) * (step(0.0f, v.xy) * 2.0f - vec2(1.0f));
	return normalize(v);
}

/** Returns a relative luminance of an input linear RGB color in the ITU-R BT.709 color space
\param RGBColor linear HDR RGB color in the ITU-R BT.709 color space
*/
float luminance(vec3 rgb)
{
	return dot(rgb, vec3(0.2126f, 0.7152f, 0.0722f));
}

float SphereSurfaceArea(float radius)
{
	return FOUR_PI * radius * radius;
}

float DiskSurfaceArea(float radius)
{
	return PI * radius * radius;
}

float QuadSurfaceArea(float radiusX, float radiusY)
{
	return 4 * radiusX * radiusY;
}

vec2 ConcentricSampleDisk(vec2 u)
{
	vec2 uOffset = u * 2.0f - 1.0f;
	if (dot(uOffset, uOffset) == 0.0f) return vec2(0.0f);

	float r     = 0.0f;
	float theta = 0.0f;

	if (abs(uOffset.x) > abs(uOffset.y))
	{
		r = uOffset.x;
		theta = PI_OVER_FOUR * uOffset.y / uOffset.x;
	}
	else
	{
		r = uOffset.y;
		theta = PI_OVER_TWO - PI_OVER_FOUR * uOffset.x / uOffset.y;
	}

	return r * vec2(cos(theta), sin(theta));
}

//These functions assume that the untransformed quad lies in the xz-plane with normal pointing in positive y direction and has a radius of 1 (side length of 2)
float QuadPDF(mat4 transform)
{
	return 1.0f / QuadSurfaceArea(length(transform[0].xyz), length(transform[2].xyz));
}

vec3 QuadNormal(mat4 transform)
{
	return (transform * vec4(0.0f, 1.0f, 0.0f, 0.0f)).xyz;
}

SShapeSample SampleQuad(mat4 transform, vec2 u)
{
	SShapeSample shapeSample;

	u = u * 2.0f - 1.0f;
	shapeSample.Position 	= (transform * vec4(u.x, 0.0f, u.y, 1.0f)).xyz; //Assume quad thickiness of 0.0f
	shapeSample.Normal		= QuadNormal(transform);
	shapeSample.PDF			= QuadPDF(transform);

	return shapeSample;
}

SShapeSample SampleQuad(vec3 position, vec3 direction, float radius, vec2 u)
{
	SShapeSample shapeSample;

	direction = normalize(direction);

	vec3 tangent;
	vec3 bitangent;
	CreateCoordinateSystem(direction, tangent, bitangent);

	u = u * 2.0f - 1.0f;
	shapeSample.Position 	= position + radius * (u.x * tangent + u.y * bitangent); //Assume quad thickiness of 0.0f
	shapeSample.Normal		= direction;
	shapeSample.PDF			= 1.0f / QuadSurfaceArea(radius, radius);

	return shapeSample;
}

SShapeSample SampleDisk(vec3 position, vec3 direction, float radius, vec2 u)
{
	SShapeSample shapeSample;

	direction = normalize(direction);

	vec3 tangent;
	vec3 bitangent;
	CreateCoordinateSystem(direction, tangent, bitangent);

	vec2 pd = ConcentricSampleDisk(u);
	shapeSample.Position 	= position + radius * (pd.x * tangent + pd.y * bitangent); //Assume disk thickiness of 0.0f
	shapeSample.Normal		= direction;
	shapeSample.PDF			= 1.0f / DiskSurfaceArea(radius);

	return shapeSample;
}

SShapeSample UniformSampleUnitSphere(vec2 u)
{
	SShapeSample shapeSample;

	float z 	= 1.0f - 2.0f * u.x;
	float r 	= sqrt(max(0.0f, 1.0f - z * z));
	float phi 	= 2.0f * PI * u.y;

	shapeSample.Position 	= vec3(r * cos(phi), r * sin(phi), z);
	shapeSample.Normal	 	= normalize(shapeSample.Position);
	shapeSample.PDF 		= 1.0f / FOUR_PI;
	return shapeSample;
}

SShapeSample UniformSampleSphereSurface(vec3 position, float radius, vec2 u)
{
	SShapeSample shapeSample = UniformSampleUnitSphere(u);

	shapeSample.Position	 = position + radius * shapeSample.Normal; //The normal as returned from UniformSampleUnitSphere can be interpreted as a point on the surface
	shapeSample.PDF 		 /= (radius * radius);
	return shapeSample;
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
	projCoords = projCoords * 0.5 + 0.5;

	if (projCoords.z > 1.0)
	{
		return 0.0f;
	}

	float closestDepth = texture(shadowMap, projCoords.xy).r;
	float currentDepth = projCoords.z;

	float bias = 0.5; 

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
