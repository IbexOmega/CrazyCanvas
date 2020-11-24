#include "../Defines.glsl"
#include "../Helpers.glsl"

struct SReflectionDesc
{
    vec3    f_div_PDF;
    float   PDF;
    vec3    w_iw;
};

struct SBxDFEval
{
    vec3    f;
    float   PDF;
}

/*
* Samples the GGX Distribution (D) using Importance Sampling
* w_so            - the outgoing direction in surface space
* alphaSqrd       - the isotropic roughness coefficient squared
* u               - a 2D sample variate
*/
vec3 Sample_w_h_GGX(vec3 w_os, float alphaSqrd, vec2 u)
{
    float phi           = u.x * TWO_PI;
    float cosThetaSqrd  = clamp((1.0f - u.y) / max(0.001f, (1.0f + (alphaSqrd - 1.0f) * u.y)), 0.0f, 1.0f); //Dividing by potentially small float, therefore, we clamp it
    float cosTheta      = sqrt(cosThetaSqrd);
    float sinTheta      = sqrt(1.0f - cosThetaSqrd);
    vec3 w_hs           = SphericalToDirection(sinTheta, cosTheta, phi);

    return w_os * FlipIfNotSameHemisphere(w_hs, w_os);
}

SBxDFEval Eval_f_Specular(float n_dot_i, float n_dot_h, float o_dot_h, float n_dot_o, vec3 albedo, float roughness, float alphaSqrd, vec3 F)
{
    float   D = Distribution(n_dot_h, alphaSqrd);
    float   G = GeometryGGX(n_dot_o, n_dot_i, roughness);

    SBxDFEval bxdfEval;
    bxdfEval.f      = D * G * F / (4.0f * n_dot_o * n_dot_i);
    bxdfEval.PDF    = D * NdotH / (4.0f * o_dot_h);
    return bxdfEval;
}

/*
* Lambertian BRDF Evaluation
*/
SBxDFEval Eval_f_Diffuse(vec3 albedo, float k_d, float cosTheta)
{
    SBxDFEval bxdfEval;
    bxdfEval.f      = k_d * albedo * INV_PI;
    bxdfEval.PDF    = cosTheta * INV_PI;
    return bxdfEval;
}

SReflectionDesc Sample_f(vec3 w_ow, vec3 w_nw, float n_dot_o, vec3 albedo, float roughness, float metallic, vec3 F0, vec2 u)
{
    SReflectionDesc reflectionDesc;
    reflectionDesc.f_div_PDF    = vec3(0.0f);
    reflectionDesc.PDF          = 0.0f;
    reflectionDesc.w_iw         = vec3(0.0f);

    //Don't sample for tangent directions
    if (n_dot_o == 0.0f) 
        return reflection;

    if (roughness == 0.0f)
    {
        //Perfect Specular Reflection, just reflect w_o around normal, delta distribution means that we set the PDF to 1
        reflectionDesc.w_iw         = reflect(-w_ow,  w_nw);
        reflectionDesc.PDF          = 1.0f;
        reflectionDesc.f            = 1.0f / max(0.001f, n_dot_o);
    }
    else
    {
        float alpha             = roughness * roughness;
        float alphaSqrd         = max(alpha * alpha, 0.0000001f);

        //We can only sample the Distribution in Surface Space so we need to create matrices to transform to/from Surface Space

        //Define Surface Coordinate System
        vec3 wX 	= vec3(0.0f);
        vec3 wY 	= vec3(0.0f);
        CreateCoordinateSystem(w_nw, wX, wY);

        //Create Transformation Matrices
        mat3 surfaceToWorld   = mat3(wX, wY, w_nw);
        mat3 worldToSurface   = transpose(surfaceToWorld);

        //Transfer to Surface Space
        vec3 w_os = worldToSurface * w_ow;

        //Sample Microfacet Normal using GGX Distribution
        vec3 w_hs = Sample_w_h_GGX(w_os, alphaSqrd, u);
        vec3 w_is = -reflect(w_os, w_hs);

        reflectionDesc.w_iw = surfaceToWorld * w_is;

        //Calculate Fresnel
        vec3 F = Fresnel(F0, cosTheta);

        //Prepare some dot products
        float n_dot_i = max(0.001f, w_is.z);
        float n_dot_h = max(0.001f, w_hs.z);
        float o_dot_h = max(0.001f, dot(w_os, w_hs));

        //Eval common BRDF
        SBxDFEval specularBRDFEval  = Eval_f_Specular(n_dot_i, n_dot_h, o_dot_h, n_dot_o, albedo, roughness, alphaSqrd, F);

        //True metallic objects don't use a lambertian BRDF
        if (metallic == 1.0f)
        {
            reflectionDesc.PDF  = specularBRDFEval.PDF;
            reflectionDesc.f    = specularBRDFEval.f;
        }
        else
        {      
            vec3 k_s = F;
            vec3 k_d = (1.0f - k_s) * (1.0f - metallic);            

            //Eval BRDF
            SBxDFEval lambertBRDFEval   = Eval_f_Diffuse(albedo, k_d, cosTheta);    

            reflectionDesc.PDF  = (lambertBRDFEval.PDF + specularBRDFEval.PDF) * 0.5f;
            reflectionDesc.f    = lambertBRDFEval.f + specularBRDFEval.f;
        }
    }

    return reflectionDesc;
}

SReflectionDesc f(vec3 w_ow, vec3 w_iw, float n_dot_o, vec3 albedo, float roughness, float metallic, vec3 F0)
{
    SReflectionDesc reflectionDesc;
    reflectionDesc.f_div_PDF    = vec3(0.0f);
    reflectionDesc.PDF          = 0.0f;
    reflectionDesc.w_iw         = w_iw;

    //Perfect Specular are delta distributions -> (mathematically) zero percent chance that w_i gives a value of f > 0 
    if (n_dot_o == 0.0f || roughness == 0.0f) 
        return reflection;

    float alpha         = roughness * roughness;
    float alphaSqrd     = max(alpha * alpha, 0.0000001f);

    //Calculate Halfway Vector
    vec3 w_hw = normalize(w_ow + w_iw);

    //Calculate Fresnel
    vec3 F = Fresnel(F0, cosTheta);
    
    //Prepare some dot products
    float n_dot_i = max(dot(w_nw, w_iw), 0.001f);
    float n_dot_h = max(dot(w_nw, w_hw), 0.001f);
    float o_dot_h = max(dot(w_ow, w_hw), 0.001f);

    //Eval common BRDF
    SBxDFEval specularBRDFEval  = Eval_f_Specular(n_dot_i, n_dot_h, o_dot_h, n_dot_o, albedo, roughness, alphaSqrd, F);

    //True metallic objects don't use a lambertian BRDF
    if (metallic == 1.0f)
    {
        reflectionDesc.PDF  = specularBRDFEval.PDF;
        reflectionDesc.f    = specularBRDFEval.f;
    }
    else
    {
        vec3 k_s = F;
        vec3 k_d = (1.0f - k_s) * (1.0f - metallic);

        //Eval BRDF
        SBxDFEval lambertBRDFEval   = Eval_f_Diffuse(albedo, k_d, cosTheta);      

        reflectionDesc.PDF  = (lambertBRDFEval.PDF + specularBRDFEval.PDF) * 0.5f;
        reflectionDesc.f    = lambertBRDFEval.f + specularBRDFEval.f;
    }

    return reflectionDesc;
}