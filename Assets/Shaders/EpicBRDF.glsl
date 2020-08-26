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
vec3 Sample_w_h_GGX(vec3 w_o, float alphaSqrd, vec2 u)
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

void Eval_f_Epic(in vec3 w_o, in vec3 w_h, in vec3 w_i, in vec3 albedo, in vec3 F_0, in float roughness, in float alphaSqrd, inout vec3 f, inout float PDF)
{
    float NdotO = max(0.0f, w_o.z);
    float NdotI = max(0.0f, w_i.z);
    float NdotH = max(0.0f, w_h.z);
    float OdotH = max(0.0f, dot(w_o, w_h));

    float   D = D(NdotH, alphaSqrd);
    float   G = G(NdotO, NdotI, roughness);
    vec3    F = F(OdotH, F_0);

    f       = D * G * F / (4.0f * NdotO * NdotI);
    PDF     = D * NdotH / (4.0f * OdotH);
}

void Eval_f_Lambert(in vec3 w_o, in vec3 w_h, in vec3 albedo, in vec3 F_0, in float metallic, in float cosTheta, inout vec3 f, inout float PDF)
{
    float OdotH     = max(0.0f, dot(w_o, w_h));
    vec3 F          = F(OdotH, F_0);
    vec3 k_D        = (1.0f - F) * (1.0f - metallic);

    f       = k_D * albedo * INV_PI;
    PDF     = cosTheta * INV_PI;
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

    if (roughness == 0.0f)
    {
        //Perfect Specular Reflection

        reflection.w_i          = vec3(-w_o.x, -w_o.y, w_o.z); //Perfect Specular Reflection, just reflect w_o around normal
        reflection.CosTheta     = max(0.0f, reflection.w_i.z);
        reflection.f            = vec3(1.0f) / reflection.CosTheta;
        reflection.PDF          = 1.0f;
    }
    else
    {
        //For materials with a roughness above 0 we use the Epic Games BRDF Model, and possibly a Lambertian BRDF Model

        vec3 F_0    = vec3(0.04f);
        F_0	        = mix(F_0, albedo, metallic);

        float alpha             = roughness * roughness;
        float alphaSqrd         = max(alpha * alpha, 0.0000001f);

         if (metallic == 1.0f)
        {
            //Metallic, no Lambertian, only Epic BRDF

            vec3 w_h                    = Sample_w_h_GGX(w_o, alphaSqrd, u.xy);
            reflection.w_i              = -reflect(w_o, w_h); //w_o is pointing out, negate 
            
            if (!IsSameHemisphere(w_o, reflection.w_i)) return reflection;
            reflection.CosTheta         = max(0.0f, reflection.w_i.z);

            vec3    epic_f            = vec3(0.0f);
            float   epic_PDF          = 0.0f;

            Eval_f_Epic(w_o, w_h, reflection.w_i, albedo, F_0, roughness, alphaSqrd, epic_f, epic_PDF);

            reflection.f        = epic_f;
            reflection.PDF      = epic_PDF;
        }
        else
        {
            //For non Perfect Specular Reflections we use a Lambertian + Epic combined BRDF with uniform sampling

            float probToSampleDiffuse   = 0.5f;
            bool sampleDiffuse          = u.z < probToSampleDiffuse;
            
            vec3 w_h                    = sampleDiffuse ? Sample_w_h_CosHemisphere(w_o, u.xy) : Sample_w_h_GGX(w_o, alphaSqrd, u.xy);
            reflection.w_i              = -reflect(w_o, w_h); //w_o is pointing out, negate 
            
            if (!IsSameHemisphere(w_o, reflection.w_i)) return reflection;
            reflection.CosTheta         = max(0.0f, reflection.w_i.z);

            /*
                Technically, we could save some computations by not returning f and PDF since:
                Incident_Light  = SampleColor * dot(w_i, w_n)
                f               = D * G * F / (4.0f * dot(w_o, w_n) * dot(w_i, w_n))
                PDF             = D * dot(w_n, w_h) / (4.0f * dot(w_o, w_h))
                    where w_n   = (0, 0, 1)
                and thus: Incident_Light * F / PDF = G * F * dot(w_o, w_h) / (dot(w_o, w_n) * dot(w_n, w_h)),
                    but we would like to evaluate the PDF
            */

            vec3    epic_f            = vec3(0.0f);
            float   epic_PDF          = 0.0f;
            vec3    lambert_f         = vec3(0.0f);    
            float   lambert_PDF       = 0.0f;

            Eval_f_Epic(w_o, w_h, reflection.w_i, albedo, F_0, roughness, alphaSqrd, epic_f, epic_PDF);
            Eval_f_Lambert(w_o, w_h, albedo, F_0, metallic, reflection.CosTheta, lambert_f, lambert_PDF);

            reflection.f        = epic_f + lambert_f;
            reflection.PDF      = (epic_PDF + lambert_PDF) * 0.5f;
        }
    }

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
    reflection.f            = vec3(0.0f);
    reflection.PDF          = 0.0f;
    reflection.w_i          = vec3(0.0f);
    reflection.CosTheta     = 0.0f;

    //Perfect Specular are delta distributions -> (mathematically) zero percent chance that w_i gives a value of f > 0 
    if (roughness == 0.0f || w_o.z <= 0.0f || w_i.z <= 0.0f) return reflection;

    //For materials with a roughness above 0 we use the Epic Games BRDF Model, and possibly a Lambertian BRDF Model

    float alpha             = roughness * roughness;
    float alphaSqrd         = max(alpha * alpha, 0.0000001f);
    vec3 w_h                = normalize(w_o + w_i);

    vec3 F_0                = vec3(0.04f);
    F_0	                    = mix(F_0, albedo, metallic);

    reflection.w_i          = w_i;
    reflection.CosTheta     = max(0.0f, w_i.z);

    if (metallic == 1.0f)
    {
        //Metallic, no Lambertian, only Epic BRDF

        vec3    epic_f            = vec3(0.0f);
        float   epic_PDF          = 0.0f;

        Eval_f_Epic(w_o, w_h, reflection.w_i, albedo, F_0, roughness, alphaSqrd, epic_f, epic_PDF);

        reflection.f        = epic_f;
        reflection.PDF      = epic_PDF;
    }
    else
    {
        //For non Perfect Specular Reflections we use a Lambertian + Epic combined BRDF with uniform sampling

        vec3    epic_f            = vec3(0.0f);
        float   epic_PDF          = 0.0f;
        vec3    lambert_f         = vec3(0.0f);    
        float   lambert_PDF       = 0.0f;

        Eval_f_Epic(w_o, w_h, reflection.w_i, albedo, F_0, roughness, alphaSqrd, epic_f, epic_PDF);
        Eval_f_Lambert(w_o, w_h, albedo, F_0, metallic, reflection.CosTheta, lambert_f, lambert_PDF);

        reflection.f        = epic_f + lambert_f;
        reflection.PDF      = (epic_PDF + lambert_PDF) * 0.5f;
    }

    return reflection;
}