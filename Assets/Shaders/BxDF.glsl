#include "Defines.glsl"
#include "Helpers.glsl"

struct SReflection
{
    float   PDF;
    vec3    f;
    vec3    w_i;
    float   CosTheta;
};

float LocalCosTheta(vec3 w)
{
    return w.z;
}

float LocalCos2Theta(vec3 w)
{
    return w.z * w.z;
}

float LocalAbsCosTheta(vec3 w)
{
    return abs(w.z);
}

float LocalSin2Theta(vec3 w)
{
    return max(0.0f, 1.0f - LocalCos2Theta(w));
}

float LocalSinTheta(vec3 w)
{
    return sqrt(LocalSin2Theta(w));
}

float LocalTanTheta(vec3 w)
{
    return LocalSinTheta(w) / LocalCosTheta(w);
}

float LocalTan2Theta(vec3 w)
{
    return LocalSin2Theta(w) / LocalCos2Theta(w);
}

float LocalCosPhi(vec3 w) 
{
    float sinTheta  = LocalSinTheta(w);
    float cosPhi    = clamp(w.x / sinTheta, -1.0f, 1.0f);
    float isValid   = step(EPSILON, abs(sinTheta));
    return isValid * cosPhi + (1.0f - isValid);
}

float LocalSinPhi(vec3 w) 
{
    float sinTheta  = LocalSinTheta(w);
    float sinPhi    = clamp(w.x / sinTheta, -1.0f, 1.0f);
    float isValid   = step(EPSILON, abs(sinTheta));
    return isValid * sinPhi + (1.0f - isValid);
}

float LocalCos2Phi(vec3 w) 
{
    return LocalCosPhi(w) * LocalCosPhi(w);
}

float LocalSin2Phi(vec3 w) 
{
    return LocalSinPhi(w) * LocalSinPhi(w);
}

/*
    See https://github.com/mmp/pbrt-v3/blob/bcac2854019683b06ca1a5b30e983816a831e481/src/core/microfacet.h 
*/
float RoughnessToAlpha(float roughness)
{
    roughness = max(roughness, EPSILON);
    float x = log(roughness);
    return 1.62142f + 0.819955f * x + 0.1734f * x * x + 0.0171201f * x * x * x + 0.000640711f * x * x * x * x;
}

/*
    Samples the Halfway Vector (w_h) for a Beckmann Distribution
    w_o             - the viewDir pointing towards the viewer in primitive local space
    alphaSqrd       - the isotropic roughness coefficient squared
    u               - a 2D sample variate
*/
vec3 Sample_w_h(vec3 w_o, float alphaSqrd, vec2 u)
{
    float logSample     = log(max(1.0f - u.x, EPSILON));
    float tan2Theta     = -alphaSqrd * logSample;
    float phi           = u.y * TWO_PI;

    float cosTheta      = 1.0f / sqrt(1.0f + tan2Theta);
    float sinTheta      = sqrt(max(0.0f, 1.0f - cosTheta * cosTheta));
    vec3 w_h            = SphericalToDirection(sinTheta, cosTheta, phi);

    return w_h * SameHemisphere(w_h, w_o);    
}

/*
    Evaluates the Beckmann Distribution function
    w_h             - the halfway vector between the viewDir and the reflectionDir in primitive local space
    alphaSqrd       - the isotropic roughness coefficient squared
*/
float BeckmannD(vec3 w_h, float alphaSqrd)
{
    float tan2Theta = LocalTan2Theta(w_h);
    if (isinf(tan2Theta)) return 0.0f;

    float cos2Theta = LocalCos2Theta(w_h);
    float cos4Theta = cos2Theta * cos2Theta;

    float alphaSqrdInv = 1.0f / alphaSqrd;

    return exp(-tan2Theta * alphaSqrdInv) / (PI * alphaSqrd * cos4Theta);
}

/*
    Evaluates the Trowbridge–Reitz Distribution function
    w_h             - the halfway vector between the viewDir and the reflectionDir in primitive local space
    alphaSqrd       - the isotropic roughness coefficient squared
*/
float TrowbridgeReitzD(vec3 w_h, float alphaSqrd)
{
    float tan2Theta = LocalTan2Theta(w_h);
    if (isinf(tan2Theta)) return 0.0f;

    float cos2Theta = LocalCos2Theta(w_h);
    float cos4Theta = cos2Theta * cos2Theta;

    float alphaSqrdInv = 1.0f / alphaSqrd;

    float e = tan2Theta / alphaSqrd;
    float e_plus_1 = e + 1;

    return 1.0f / (PI * alphaSqrd * cos4Theta * e_plus_1 * e_plus_1);
}

float Lambda(vec3 w, float alpha)
{
    float absTanTheta = abs(LocalTanTheta(w));
    if (isinf(absTanTheta)) return 0.0f;

    float a = 1.0f / (alpha * absTanTheta);
    return (1.0f - step(1.6f, a)) * (1.0f - 1.259f * a + 0.396f * a * a) / (3.535f * a + 2.181f * a * a);
}

float BeckmannG1(vec3 w, float alpha)
{
    return 1.0f / (1.0f + Lambda(w, alpha));
}

float BeckmannG(vec3 w_o, vec3 w_i, float alpha)
{
    return 1.0f / (1.0f + Lambda(w_o, alpha) + Lambda(w_i, alpha));
}

/*
    Calculates the value of the PDF with w_h which is in primitive local space
    w_h             - the halfway vector between the viewDir and the reflectionDir in primitive local space
    alphaSqrd       - the isotropic roughness coefficient squared
*/
float HalfwayPDF(vec3 w_h, float alphaSqrd)
{
    return BeckmannD(w_h, alphaSqrd) * LocalAbsCosTheta(w_h);
}

float PDF(vec3 w_o, vec3 w_h, float alphaSqrd)
{
    return HalfwayPDF(w_h, alphaSqrd) / (4.0f * (w_o.x * w_h.x + w_o.y * w_h.y + w_o.z * w_h.z));
}

vec3 Microfacet_f(vec3 w_o, vec3 w_h, vec3 w_i, float alpha, float alphaSqrd, vec3 F_0)
{
    float cosThetaO = LocalAbsCosTheta(w_o);
    float cosThetaI = LocalAbsCosTheta(w_i);
    
    if (cosThetaO == 0 || cosThetaI == 0 || dot(w_h, w_h) < EPSILON) return vec3(0.0f);

    vec3 F = Fresnel(F_0, dot(w_i, w_h));
    float D = BeckmannD(w_h, alphaSqrd);
    float G = BeckmannG(w_o, w_i, alpha);

    return F * D * G / (4.0f * cosThetaI * cosThetaO);
}

/*
    Samples the BRDF with viewDir w_o which is in primitive local space
    w_o             - the viewDir pointing towards the viewer in primitive local space
    alpha           - the isotropic roughness coefficient
    F_0             - the reflection coefficient for light incoming parallel to the normal
    u               - a 2D sample variate
*/
SReflection Sample_f(vec3 w_o, float alpha, vec3 F_0, vec2 u)
{
    float alphaSqrd         = alpha * alpha;
    vec3 w_h                = normalize(Sample_w_h(w_o, alphaSqrd, u));
    vec3 w_i                = normalize(reflect(w_o, w_h));

    SReflection reflection;
    reflection.PDF          = 0.0f;
    reflection.f            = vec3(0.0f);
    reflection.w_i          = vec3(0.0f);
    reflection.CosTheta     = 0.0f;

    if (SameHemisphere(w_o, w_i) < 0.0f) return reflection;

    reflection.PDF          = PDF(w_o, w_h, alphaSqrd);
    reflection.f            = Microfacet_f(w_o, w_h, w_i, alpha, alphaSqrd, F_0);
    reflection.w_i          = w_i;
    reflection.CosTheta     = abs(dot(w_h, w_i));

    return reflection;
}

/*
    Evaluates the BRDF with viewDir w_o and reflectionDir w_i which is in primitive local space
    w_o             - the viewDir pointing towards the viewer in primitive local space
    w_i             - the reflDir pointing towards the reflected direction
    alpha           - the isotropic roughness coefficient
    F_0             - the reflection coefficient for light incoming parallel to the normal
*/
SReflection f(vec3 w_o, vec3 w_i, float alpha, vec3 F_0)
{
    SReflection reflection; 
    reflection.PDF          = 0.0f;
    reflection.f            = vec3(0.0f);
    reflection.w_i          = vec3(0.0f);
    reflection.CosTheta     = 0.0f;

    if (SameHemisphere(w_o, w_i) < 0.0f) return reflection;

    float alphaSqrd         = alpha * alpha;
    vec3 w_h                = normalize(w_o + w_i);

    reflection.PDF          = PDF(w_o, w_h, alphaSqrd);
    reflection.f            = Microfacet_f(w_o, w_h, w_i, alpha, alphaSqrd, F_0);
    reflection.w_i          = w_i;
    reflection.CosTheta     = abs(dot(w_h, w_i));

    return reflection;
}