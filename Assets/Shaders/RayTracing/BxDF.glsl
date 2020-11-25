#include "../Defines.glsl"
#include "../Helpers.glsl"

struct SReflectionDesc
{
    vec3    f;
    float   PDF;
    vec3    f_Specular;
    float   PDF_Specular;
    vec3    w_iw;
};

struct SBxDFEval
{
    vec3    f;
    float   PDF;
};

/*
* Samples the GGX Distribution (D) using Importance Sampling
* w_so            - the outgoing direction in surface space
* alphaSqrd       - the isotropic roughness coefficient squared
* u               - a 2D sample variate
*/
vec3 Sample_w_h_GGX(vec3 w_os, float alphaSqrd, vec2 u)
{

    float phi           = u.x * TWO_PI;
    float cosThetaSqrd  = min((1.0f - u.y) / max(0.001f, (1.0f + (alphaSqrd - 1.0f) * u.y)), 1.0f); //Dividing by potentially small float, therefore, we clamp it
    //Truncate some cosThetaSqrd to avoid Samplig GGX Tail
    cosThetaSqrd = mix(cosThetaSqrd, 1.0f, BRDF_TAIL_TRUNCATION_BIAS);
    float cosTheta      = sqrt(cosThetaSqrd);
    float sinTheta      = sqrt(1.0f - cosThetaSqrd);
    vec3 w_hs           = SphericalToDirection(sinTheta, cosTheta, phi);

    return w_hs * FlipIfNotSameHemisphere(w_hs, w_os);
}

vec3 Sample_w_h_CosHemisphere(vec2 u)
{
    u = u * 2.0f - 1.0f;
    if (u.x == 0.0f || u.y == 0.0f) return vec3(0.0f, 0.0f, 1.0f);

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

SBxDFEval Eval_f_Specular(float n_dot_i, float n_dot_h, float o_dot_h, float n_dot_o, vec3 albedo, float roughness, float alphaSqrd, vec3 F)
{
    float   D = Distribution(n_dot_h, alphaSqrd);
    float   G = Geometry(n_dot_o, n_dot_i, roughness);

    SBxDFEval bxdfEval;
    bxdfEval.f      = D * G * F / (4.0f * n_dot_o * n_dot_i);
    bxdfEval.PDF    = D * n_dot_h / (4.0f * o_dot_h);
    return bxdfEval;
}

/*
* Lambertian BRDF Evaluation
*/
SBxDFEval Eval_f_Diffuse(vec3 albedo, vec3 k_d, float cosTheta)
{
    SBxDFEval bxdfEval;
    bxdfEval.f      = k_d * albedo * INV_PI;
    bxdfEval.PDF    = cosTheta * INV_PI;
    return bxdfEval;
}

SReflectionDesc Sample_f(vec3 w_ow, vec3 w_nw, float n_dot_o, vec3 albedo, float roughness, float metallic, vec3 F0, vec4 u)
{
    SReflectionDesc reflectionDesc;
    reflectionDesc.f                = vec3(0.0f);
    reflectionDesc.PDF              = 0.0f;
    reflectionDesc.f_Specular       = vec3(0.0f);
    reflectionDesc.PDF_Specular     = 0.0f;
    reflectionDesc.w_iw             = vec3(0.0f);

    //Don't sample for tangent directions
    if (n_dot_o == 0.0f) 
        return reflectionDesc;

    if (roughness == 0.0f)
    {
        //Perfect Specular Reflection, just reflect w_o around normal, delta distribution means that we set the PDF to 1
        reflectionDesc.w_iw         = reflect(-w_ow,  w_nw);
        reflectionDesc.PDF          = 1.0f;
        reflectionDesc.f            = vec3(1.0f) / max(0.001f, n_dot_o);
        reflectionDesc.PDF_Specular = reflectionDesc.PDF;
        reflectionDesc.f_Specular   = reflectionDesc.f;
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

        //True metallic objects don't use a lambertian BRDF
        if (metallic == 1.0f)
        {
            //Sample Microfacet Normal using GGX Distribution
            vec3 w_hs = Sample_w_h_GGX(w_os, alphaSqrd, u.xy);
            vec3 w_is = reflect(-w_os, w_hs);

            reflectionDesc.w_iw = surfaceToWorld * w_is;

            //Prepare some dot products
            float n_dot_i = max(0.001f, w_is.z);
            float n_dot_h = max(0.001f, w_hs.z);
            float o_dot_h = max(0.001f, dot(w_os, w_hs));

            //Calculate Fresnel
            vec3 F = Fresnel(F0, n_dot_i);

            //Eval common BRDF
            SBxDFEval specularBRDFEval  = Eval_f_Specular(n_dot_i, n_dot_h, o_dot_h, n_dot_o, albedo, roughness, alphaSqrd, F);

            reflectionDesc.PDF          = specularBRDFEval.PDF;
            reflectionDesc.f            = specularBRDFEval.f;
            reflectionDesc.PDF_Specular = specularBRDFEval.PDF;
            reflectionDesc.f_Specular   = specularBRDFEval.f;
        }
        else
        {     
            //Sample Microfacet Normal using GGX Distribution
            {
                vec3 w_hs = Sample_w_h_GGX(w_os, alphaSqrd, u.xy);
                vec3 w_is = reflect(-w_os, w_hs);

                reflectionDesc.w_iw = surfaceToWorld * w_is;

                //Prepare some dot products
                float n_dot_i = max(0.001f, w_is.z);
                float n_dot_h = max(0.001f, w_hs.z);
                float o_dot_h = max(0.001f, dot(w_os, w_hs));

                //Calculate Fresnel
                vec3 F = Fresnel(F0, n_dot_i);

                //Eval common BRDF
                SBxDFEval specularBRDFEval  = Eval_f_Specular(n_dot_i, n_dot_h, o_dot_h, n_dot_o, albedo, roughness, alphaSqrd, F);

                vec3 k_s = F;
                vec3 k_d = (1.0f - k_s) * (1.0f - metallic);            

                //Eval BRDF
                SBxDFEval lambertBRDFEval   = Eval_f_Diffuse(albedo, k_d, n_dot_i);    

                reflectionDesc.PDF_Specular     = specularBRDFEval.PDF;
                reflectionDesc.f_Specular       = specularBRDFEval.f;
                reflectionDesc.PDF              = (lambertBRDFEval.PDF + specularBRDFEval.PDF) * 0.5f;
                reflectionDesc.f                = lambertBRDFEval.f + specularBRDFEval.f;
            }

            /*
            * Dielectrics should have a possibility of sampling the diffuse BRDF. 
            * However, we want to avoid this since it creates lots of noise. 
            * What we do instead is that we pretend that we send a ray in the same direction as sampling of the Specular BRDF gave.
            * We can do this by writing:
            *   (l_i * cosTheta * (f(specular_sample) / pdf(specular_sample)) + l_i * cosTheta * (f(diffuse_sample) / pdf(diffuse_sample))) / N =
            *   (l_i * cosTheta * (f(specular_sample) / pdf(specular_sample) + f(diffuse_sample) / pdf(diffuse_sample))) / N =
            *   (l_i * cosTheta * ((f(specular_sample) * pdf(diffuse_sample) + f(diffuse_sample) * pdf(specular_sample)) / (pdf(specular_sample) * pdf(diffuse_sample)))) / N
            * and not overwriting w_iw with the diffuse sample direction
            */       
            {
                vec3 w_hs = Sample_w_h_CosHemisphere(u.zw);
                vec3 w_is = reflect(-w_os, w_hs);

                //Prepare some dot products
                float n_dot_i = max(0.001f, w_is.z);
                float n_dot_h = max(0.001f, w_hs.z);
                float o_dot_h = max(0.001f, dot(w_os, w_hs));

                //Calculate Fresnel
                vec3 F = Fresnel(F0, n_dot_i);

                //Eval common BRDF
                SBxDFEval specularBRDFEval  = Eval_f_Specular(n_dot_i, n_dot_h, o_dot_h, n_dot_o, albedo, roughness, alphaSqrd, F);

                vec3 k_s = F;
                vec3 k_d = (1.0f - k_s) * (1.0f - metallic);            

                //Eval BRDF
                SBxDFEval lambertBRDFEval   = Eval_f_Diffuse(albedo, k_d, n_dot_i);    

                vec3 f      = (lambertBRDFEval.f + specularBRDFEval.f);
                float PDF   = (lambertBRDFEval.PDF + specularBRDFEval.PDF) * 0.5f;

                //We're taking 2 samples so we need to divide by 2 (multiply by 0.5)
                reflectionDesc.f                = ((f * reflectionDesc.PDF) + (reflectionDesc.f * PDF)) * 0.5f;
                reflectionDesc.PDF              = PDF * reflectionDesc.PDF;
            }
        }
    }

    return reflectionDesc;
}

SReflectionDesc f(vec3 w_ow, vec3 w_iw, vec3 w_nw, float n_dot_o, vec3 albedo, float roughness, float metallic, vec3 F0)
{
    SReflectionDesc reflectionDesc;
    reflectionDesc.f        = vec3(0.0f);
    reflectionDesc.PDF      = 0.0f;
    reflectionDesc.w_iw     = w_iw;

    //Perfect Specular are delta distributions -> (mathematically) zero percent chance that w_i gives a value of f > 0 
    if (n_dot_o == 0.0f || roughness == 0.0f) 
        return reflectionDesc;

    float alpha         = roughness * roughness;
    float alphaSqrd     = max(alpha * alpha, 0.0000001f);

    //Calculate Halfway Vector
    vec3 w_hw = normalize(w_ow + w_iw);
    
    //Prepare some dot products
    float n_dot_i = max(dot(w_nw, w_iw), 0.001f);
    float n_dot_h = max(dot(w_nw, w_hw), 0.001f);
    float o_dot_h = max(dot(w_ow, w_hw), 0.001f);

    //Calculate Fresnel
    vec3 F = Fresnel(F0, n_dot_i);

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
        SBxDFEval lambertBRDFEval   = Eval_f_Diffuse(albedo, k_d, n_dot_i);

        reflectionDesc.PDF  = (lambertBRDFEval.PDF + specularBRDFEval.PDF) * 0.5f;
        reflectionDesc.f    = lambertBRDFEval.f + specularBRDFEval.f;
    }

    return reflectionDesc;
}