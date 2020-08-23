#include "Defines.glsl"
#include "Helpers.glsl"

struct SReflection
{
    float   PDF;
    vec3    f;
    vec3    w_i;
    float   CosTheta;
};

/*
	GGX Distribution function
*/
float D(float NdotH, float alphaSqrd)
{   
	float denom = ((NdotH * NdotH) * (alphaSqrd - 1.0f)) + 1.0f;
	return alphaSqrd / (PI * denom * denom);
}

float G1(float NdotV, float k)
{
    return NdotV / (NdotV * (1.0f - k) + k);
}

float G(float NdotO, float NdotI, float roughness)
{
    float r_1 = roughness + 1.0f;
    float k = r_1 * r_1 / 8.0f;
    return G1(NdotO, k) * G1(NdotI, k);
}

vec3 F(float OdotH, vec3 F_0)
{
    float exponent = (-5.55473f * OdotH - 6.98316f) * OdotH; //See: https://seblagarde.wordpress.com/2012/06/03/spherical-gaussien-approximation-for-blinn-phong-phong-and-fresnel/ 
    return F_0 + (1.0f - F_0) * exp2(exponent);
}

/*
    Samples the GGX Distribution (D) using Importance Sampling (in the future, orientation of Microfacet Normals should be included for faster convergence),
    also consider that this Ditribution is used only in the evaluation of the specular BRDF and does thus not consider the diffuse BRDF
    w_o             - the outgoing direction in primitive local space
    alphaSqrd       - the isotropic roughness coefficient squared
    u               - a 2D sample variate
*/
vec3 Sample_w_h_SimpleSpecular(vec3 w_o, float alphaSqrd, vec2 u)
{
    float phi           = u.x * TWO_PI;
    float cosThetaSqrd  = (1.0f - u.y) / (1.0f + (alphaSqrd - 1.0f) * u.y);
    float cosTheta      = sqrt(cosThetaSqrd);
    float sinTheta      = sqrt(1.0f - cosThetaSqrd);
    vec3 w_h            = SphericalToDirection(sinTheta, cosTheta, phi);

    return w_h * FlipIfNotSameHemisphere(w_h, w_o);    
}

vec3 Sample_w_h_CosHemisphere(vec3 w_o, vec2 u)
{
    u = u * 2.0f - 1.0f;
    if (dot(u, vec2(1.0f)) == 0.0f) return vec3(0.0f, 0.0f, 1.0f);

    float r     = 0.0f;
    float theta = 0.0f;

    if (abs(u.x) > abs(u.y))
    {
        r = u.x;
        theta = PI_OVER_FOUR * u.y / u.x;
    }
    else
    {
        r = u.y;
        theta = PI_OVER_TWO - PI_OVER_FOUR * u.x / u.y;
    }

    vec2 d = r * vec2(cos(theta), sin(theta));
    float z = sqrt(max(0.0f, 1.0f - dot(d, d)));
    return vec3(d.x, d.y, z);
}

void Microfacet_f(in vec3 w_o, in vec3 w_h, in vec3 w_i, in vec3 albedo, in float metallic, in float roughness, in float alphaSqrd, inout vec3 f, inout float PDF)
{
    vec3 F_0    = vec3(0.04f);
	F_0	        = mix(F_0, albedo, metallic);

    float NdotO = max(0.01f, w_o.z);
    float NdotI = max(0.0f, w_i.z);
    float NdotH = max(0.0f, w_h.z);
    float OdotH = max(0.0f, dot(w_o, w_h));

    float   D = D(NdotH, alphaSqrd);
    float   G = G(NdotO, NdotI, roughness);
    vec3    F = F(OdotH, F_0);

    vec3 specular_f = D * G * F / (4.0f * NdotO * NdotI);
    
    f       = specular_f;
    PDF     = D * NdotH / (4.0f * OdotH);
}

void Lambertian_f(in vec3 w_o, in vec3 w_h, in vec3 albedo, in float metallic, in float cosTheta, inout vec3 f, inout float PDF)
{
    vec3 F_0        = vec3(0.04f);
	F_0	            = mix(F_0, albedo, metallic);
    float OdotH     = max(0.0f, dot(w_o, w_h));

    vec3 F          = F(OdotH, F_0);

    vec3 k_D        = (1.0f - F) * (1.0f - metallic);
    vec3 diffuse_f  = k_D * albedo;

    f       = diffuse_f * INV_PI;
    PDF     = cosTheta * INV_PI;
}

void Eval_f(vec3 w_o, vec3 w_h, vec3 w_i, vec3 albedo, float metallic, float roughness, float alphaSqrd, inout SReflection reflection)
{
    reflection.w_i          = w_i;
    reflection.CosTheta     = max(0.0f, w_i.z);

    vec3 diffuse_f          = vec3(0.0f);
    vec3 specular_f         = vec3(0.0f);
    float diffuse_PDF       = 0.0f;
    float specular_PDF      = 0.0f;

    Lambertian_f(w_o, w_h, albedo, metallic, reflection.CosTheta, diffuse_f, diffuse_PDF);
    Microfacet_f(w_o, w_h, w_i, albedo, metallic, roughness, alphaSqrd, specular_f, specular_PDF);
    
    reflection.f    = diffuse_f + specular_f;
    reflection.PDF  = (diffuse_PDF + specular_PDF) * 0.5f;
}

/*
    Samples the BRDF with the outgoing direction w_o which is in primitive local space
    w_o             - the outgoing direction in primitive local space
    albedo          - the albedo of the surface
    metallic        - the "metallicness" of the surface
    roughness       - the isotropic roughness of the surface, in the range (0, 1]
    u               - a 3D sample variate
*/
SReflection Sample_f(vec3 w_o, vec3 albedo, float metallic, float roughness, vec3 u)
{
    SReflection reflection;
    reflection.PDF          = 0.0f;
    reflection.f            = vec3(0.0f);
    reflection.w_i          = vec3(0.0f);
    reflection.CosTheta     = 0.0f;

    if (w_o.z <= 0.0f)  return reflection;

    float alpha             = roughness * roughness;
    float alphaSqrd         = max(alpha * alpha, 0.0000001f);

    bool sampleDiffuse      = u.z < 0.5f;
    
    vec3 w_h                = sampleDiffuse ? Sample_w_h_CosHemisphere(w_o, u.xy) : Sample_w_h_SimpleSpecular(w_o, alphaSqrd, u.xy);
    vec3 w_i                = -reflect(w_o, w_h); //w_o is pointing out, negate reflect
    //vec3 w_i                = 2.0f * dot(w_o, w_h) * w_h - w_o;

    if (!IsSameHemisphere(w_o, w_i)) return reflection;

    //w_i = w_i * SameHemisphere(w_i, w_o);  

    /*
        Technically, we could save some computations by not returning f and PDF since:
        Incident_Light  = SampleColor * dot(w_i, w_n)
        f               = D * G * F / (4.0f * dot(w_o, w_n) * dot(w_i, w_n))
        PDF             = D * dot(w_n, w_h) / (4.0f * dot(w_o, w_h))
            where w_n   = (0, 0, 1)
        and thus: Incident_Light * F / PDF = G * F * dot(w_o, w_h) / (dot(w_o, w_n) * dot(w_n, w_h)),
            but we would like to evaluate the PDF
    */

    Eval_f(w_o, w_h, w_i, albedo, metallic, roughness, alphaSqrd, reflection);
    return reflection;
}

/*
    Evaluates the BRDF with viewDir w_o and reflectionDir w_i which is in primitive local space
    w_o             - the viewDir pointing towards the viewer in primitive local space
    w_i             - the reflDir pointing towards the reflected direction
    albedo          - the albedo of the surface
    metallic        - the "metallicness" of the surface
    roughness       - the isotropic roughness, in the range (0, 1]
*/
SReflection f(vec3 w_o, vec3 w_i, vec3 albedo, float metallic, float roughness)
{
    SReflection reflection; 
    reflection.PDF          = 0.0f;
    reflection.f            = vec3(0.0f);
    reflection.w_i          = vec3(0.0f);
    reflection.CosTheta     = 0.0f;

    if (w_o.z <= 0.0f || !IsSameHemisphere(w_o, w_i)) return reflection;

    float alpha             = roughness * roughness;
    float alphaSqrd         = max(alpha * alpha, 0.0000001f);
    vec3 w_h                = normalize(w_o + w_i);

    //w_i = w_i * SameHemisphere(w_i, w_o);  
    Eval_f(w_o, w_h, w_i, albedo, metallic, roughness, alphaSqrd, reflection);
    return reflection;
}