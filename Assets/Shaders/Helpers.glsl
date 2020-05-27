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

SRayDirections CalculateRayDirections(vec3 hitPosition, vec3 normal, vec3 cameraPosition, mat4 cameraViewInv)
{
	vec4 u_CameraOrigin = cameraViewInv * vec4(0.0f, 0.0f, 0.0f, 1.0f);
	vec3 origDirection = normalize(hitPosition - cameraPosition);

    SRayDirections rayDirections;
	rayDirections.ReflDir = reflect(origDirection, normal);
	rayDirections.ViewDir = -origDirection;
    return rayDirections;
}

vec4 bilateralBlur13Roughness(sampler2D image, vec4 centerColor, vec2 texCoords, vec2 direction, float roughness) 
{
    vec3 color = vec3(0.0f);
    vec2 resolution = textureSize(image, 0);
    float roughnessFactor = pow(0.5f + roughness, 2.0f);
    roughnessFactor = 1.0f;
    vec2 off1 = roughnessFactor * vec2(1.411764705882353f)  * direction / resolution;
    vec2 off2 = roughnessFactor * vec2(3.2941176470588234f) * direction / resolution;
    vec2 off3 = roughnessFactor * vec2(5.176470588235294f)  * direction / resolution;

    vec4 color0 = texture(image, texCoords + off1);
    vec4 color1 = texture(image, texCoords - off1);
    vec4 color2 = texture(image, texCoords + off2);
    vec4 color3 = texture(image, texCoords - off2);
    vec4 color4 = texture(image, texCoords + off3);
    vec4 color5 = texture(image, texCoords - off3);

    const float denom = sqrt(3.0f);
    float closenessCoeff0 = 1.0f - distance(centerColor, color0) / denom;
    float closenessCoeff1 = 1.0f - distance(centerColor, color1) / denom;
    float closenessCoeff2 = 1.0f - distance(centerColor, color2) / denom;
    float closenessCoeff3 = 1.0f - distance(centerColor, color3) / denom;
    float closenessCoeff4 = 1.0f - distance(centerColor, color4) / denom;
    float closenessCoeff5 = 1.0f - distance(centerColor, color5) / denom;

    float weight0 = color0.a /** closenessCoeff0*/ * 0.2969069646728344f;
    float weight1 = color1.a /** closenessCoeff1*/ * 0.2969069646728344f;
    float weight2 = color2.a /** closenessCoeff2*/ * 0.09447039785044732f;
    float weight3 = color3.a /** closenessCoeff3*/ * 0.09447039785044732f;
    float weight4 = color4.a /** closenessCoeff4*/ * 0.010381362401148057f;
    float weight5 = color5.a /** closenessCoeff5*/ * 0.010381362401148057f;

    color += centerColor.rgb * 0.1964825501511404f;
    color += color0.rgb * weight0;
    color += color1.rgb * weight1;
    color += color2.rgb * weight2;
    color += color3.rgb * weight3;
    color += color4.rgb * weight4;
    color += color5.rgb * weight5;

    float normalization = 0.0f;
    normalization += weight0;
    normalization += weight1;
    normalization += weight2;
    normalization += weight3;
    normalization += weight4;
    normalization += weight5;

    return vec4(color.rgb, 1.0f) / normalization;
    //return centerColor;
}

vec3 blur5(sampler2D image, vec4 centerColor, vec2 texCoords, vec2 normalizedDirection) 
{
    vec3 color = vec3(0.0f);
    vec2 off1 = vec2(1.3333333333333333f) * normalizedDirection;
    color += centerColor.rgb * 0.29411764705882354f;

    vec4 color0 = texture(image, texCoords + off1);
    vec4 color1 = texture(image, texCoords - off1);

    float factor0 = color0.a >= 1.0f ? 1.0f : 0.0f;
    float factor1 = color1.a >= 1.0f ? 1.0f : 0.0f;

    color += color0.rgb * factor0 * 0.35294117647058826f;
    color += color1.rgb * factor1 * 0.35294117647058826f;
    return color; 
}

vec3 blur9(sampler2D image, vec4 centerColor, vec2 texCoords, vec2 normalizedDirection) 
{
    vec3 color = vec3(0.0f);
    vec2 off1 = vec2(1.3846153846f) * normalizedDirection;
    vec2 off2 = vec2(3.2307692308f) * normalizedDirection;
    color += centerColor.rgb * 0.2270270270f;
    color += texture(image, texCoords + off1).rgb * 0.3162162162f;
    color += texture(image, texCoords - off1).rgb * 0.3162162162f;
    color += texture(image, texCoords + off2).rgb * 0.0702702703f;
    color += texture(image, texCoords - off2).rgb * 0.0702702703f;
    return color;
}

vec3 blur13(sampler2D image, vec4 centerColor, vec2 texCoords, vec2 normalizedDirection) 
{
    vec3 color = vec3(0.0f);
    vec2 off1 = vec2(1.411764705882353f) * normalizedDirection;
    vec2 off2 = vec2(3.2941176470588234f) * normalizedDirection;
    vec2 off3 = vec2(5.176470588235294f) * normalizedDirection;
    color += centerColor.rgb * 0.1964825501511404f;
    color += texture(image, texCoords + off1).rgb * 0.2969069646728344f;
    color += texture(image, texCoords - off1).rgb * 0.2969069646728344f;
    color += texture(image, texCoords + off2).rgb * 0.09447039785044732f;
    color += texture(image, texCoords - off2).rgb * 0.09447039785044732f;
    color += texture(image, texCoords + off3).rgb * 0.010381362401148057f;
    color += texture(image, texCoords - off3).rgb * 0.010381362401148057f;
    return color;
}

vec4 blur(sampler2D image, vec4 centerColor, vec2 texCoords, vec2 direction, float gamma)
{
    const float barrier0 = 1.0f / 3.0f;
    const float barrier1 = 2.0f / 3.0f;

    vec2 imageResolution = textureSize(image, 0);
    vec2 normalizedDirection = direction / imageResolution;

    vec3 bottomBlur = vec3(0.0f);
    vec3 topBlur = vec3(0.0f);
    float alpha = 0.0f;

    return vec4(blur5(image, centerColor, texCoords, normalizedDirection), centerColor.a);

    // bottomBlur = blur5(image, centerColor, texCoords, normalizedDirection);
    // topBlur = blur9(image, centerColor, texCoords, normalizedDirection);
    // alpha = gamma;

    // if (gamma < barrier0)
    // {
    //     bottomBlur = centerColor.rgb;
    //     topBlur = blur5(image, centerColor, texCoords, normalizedDirection);
    //     alpha = gamma * 3.0f;
    // }
    // else if (gamma < barrier1)
    // {
    //     bottomBlur = blur5(image, centerColor, texCoords, normalizedDirection);
    //     topBlur = blur9(image, centerColor, texCoords, normalizedDirection);
    //     alpha = (gamma - barrier0) * 3.0f;
    // }
    // else
    // {
    //     bottomBlur = blur9(image, centerColor, texCoords, normalizedDirection);
    //     topBlur = blur13(image, centerColor, texCoords, normalizedDirection);
    //     alpha = (gamma - barrier1) * 3.0f;
    // }

    //return vec4(mix(bottomBlur, topBlur, alpha), 1.0f);
    //return centerColor;
}

float normpdf(in float x, in float sigma)
{
	return 0.39894f * exp(-0.5f * x * x / (sigma * sigma)) / sigma;
}

float normpdf3(in vec3 v, in float sigma)
{
	return 0.39894f * exp(-0.5f * dot(v, v) / (sigma * sigma)) / sigma;
}

vec4 bilateralBlur(sampler2D image, vec4 centerColor, vec2 texCoords, vec2 normalizedDirection)
{
    //0.24196 0.39894 0.24196
    //BZ = 3.9894
    const float SIGMA = 10.0f;
    const float BSIGMA = 0.1f;

    const float centerWeight = normpdf(0.0f, SIGMA);
    const float neighborWeight = normpdf(1.0f, SIGMA);
    const float bZ = normpdf(0.0f, BSIGMA);

    vec4 negSample = texture(image, texCoords - normalizedDirection);
    vec4 posSample = texture(image, texCoords + normalizedDirection);

    float centerFactor = pow(centerWeight, 3) * bZ;
    float negFactor = normpdf3(negSample.rgb - centerColor.rgb, SIGMA) * bZ * neighborWeight * neighborWeight;
    float posFactor = normpdf3(posSample.rgb - centerColor.rgb, SIGMA) * bZ * neighborWeight * neighborWeight;

    float Z = negFactor + centerFactor + posFactor;

    vec3 finalColor = negSample.rgb * negFactor + centerColor.rgb * centerFactor + posSample.rgb * posFactor;
    return vec4(finalColor / Z, centerColor.a);
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

float gold_noise3(vec3 x, float seed, float min, float max)
{
    const float GOLD_PHI = 1.61803398874989484820459 * 00000.1; // Golden Ratio   
    const float GOLD_PI  = 3.14159265358979323846264 * 00000.1; // PI
    const float GOLD_SQ2 = 1.41421356237309504880169 * 10000.0; // Square Root of Two
    const float GOLD_E   = 2.71828182846;

    return mix(min, max, fract(tan(distance(x * (seed + GOLD_PHI), vec3(GOLD_PHI, GOLD_PI, GOLD_E))) * GOLD_SQ2) * 0.5f + 0.5f);
}

void CreateCoordinateSystem(in vec3 N, out vec3 Nt, out vec3 Nb) 
{ 
    if (abs(N.x) > abs(N.y)) 
        Nt = vec3(N.z, 0, -N.x) / sqrt(N.x * N.x + N.z * N.z); 
    else 
        Nt = vec3(0, -N.z, N.y) / sqrt(N.y * N.y + N.z * N.z); 
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

#endif
