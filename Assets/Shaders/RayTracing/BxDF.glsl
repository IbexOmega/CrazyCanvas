#include "../Defines.glsl"
#include "../Helpers.glsl"
#include "../Reflections.glsl"

struct SBxDFSample
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

// Input Ve: view direction
// Input alpha_x, alpha_y: roughness parameters
// Input U1, U2: uniform random numbers
// Output Ne: normal sampled with PDF D_Ve(Ne) = G1(Ve) * max(0, dot(Ve, Ne)) * D(Ne) / Ve.z
vec3 Sample_w_h_GGXVNDF(vec3 Ve, float alpha, vec2 u)
{
    // Section 3.2: transforming the view direction to the hemisphere configuration
    vec3 Vh = normalize(vec3(alpha * Ve.x, alpha * Ve.y, Ve.z));
    // Section 4.1: orthonormal basis (with special case if cross product is zero)
    float lensq = Vh.x * Vh.x + Vh.y * Vh.y;
    vec3 T1 = lensq > 0 ? vec3(-Vh.y, Vh.x, 0) * inversesqrt(lensq) : vec3(1,0,0);
    vec3 T2 = cross(Vh, T1);
    // Section 4.2: parameterization of the projected area
    float r = sqrt(u.x);
    float phi = 2.0 * PI * u.y;
    float t1 = r * cos(phi);
    float t2 = r * sin(phi);
    float s = 0.5 * (1.0 + Vh.z);
    t2 = (1.0 - s)*sqrt(1.0 - t1*t1) + s*t2;
    // Section 4.3: reprojection onto hemisphere
    vec3 Nh = t1*T1 + t2*T2 + sqrt(max(0.0, 1.0 - t1*t1 - t2*t2))*Vh;
    // Section 3.4: transforming the normal back to the ellipsoid configuration
    vec3 Ne = normalize(vec3(alpha * Nh.x, alpha * Nh.y, max(0.0, Nh.z)));
    return Ne;
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
    float cosThetaSqrd  = min((1.0f - u.y) / max(0.001f, (1.0f + (alphaSqrd - 1.0f) * u.y)), 1.0f); //Dividing by potentially small float, therefore, we clamp it
    //Truncate some cosThetaSqrd to avoid Samplig GGX Tail
    cosThetaSqrd        = mix(cosThetaSqrd, 1.0f, BRDF_TAIL_TRUNCATION_BIAS);
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

SBxDFEval Eval_f_Specular_GGX_Sampled(float n_dot_i, float n_dot_h, float o_dot_h, float n_dot_o, vec3 albedo, float roughness, float alphaSqrd, vec3 F)
{
    float   D = Distribution(n_dot_h, alphaSqrd);
    float   G = Geometry(n_dot_o, n_dot_i, roughness);

    SBxDFEval bxdfEval;
    bxdfEval.f      = D * G * F / (4.0f * n_dot_o * n_dot_i);
    bxdfEval.PDF    = D * n_dot_h / (4.0f * o_dot_h);
    return bxdfEval;
}

SBxDFEval Eval_f_Specular_GGXVNDF_Sampled(float n_dot_i, float n_dot_h, float o_dot_h, float n_dot_o, vec3 albedo, float roughness, float alphaSqrd, vec3 F)
{
    float   D = Distribution(n_dot_h, alphaSqrd);
    float   G = Geometry(n_dot_o, n_dot_i, roughness);

    SBxDFEval bxdfEval;
    bxdfEval.f      = D * G * F / (4.0f * n_dot_o * n_dot_i);
    bxdfEval.PDF    = D / (4.0f * o_dot_h);
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

SBxDFSample Sample_f(vec3 w_ow, vec3 w_nw, float n_dot_o, vec3 albedo, float roughness, float metallic, vec3 F0, vec4 u)
{
    SBxDFSample bxdfSample;
    bxdfSample.f                = vec3(0.0f);
    bxdfSample.PDF              = 0.0f;
    bxdfSample.f_Specular       = vec3(0.0f);
    bxdfSample.PDF_Specular     = 0.0f;
    bxdfSample.w_iw             = vec3(0.0f);

    //Don't sample for tangent directions
    if (n_dot_o == 0.0f) 
        return bxdfSample;

    if (roughness == 0.0f)
    {
        //Perfect Specular Reflection, just reflect w_o around normal, delta distribution means that we set the PDF to 1
        bxdfSample.w_iw         = reflect(-w_ow,  w_nw);
        bxdfSample.PDF          = 1.0f;
        bxdfSample.f            = vec3(1.0f) / max(0.001f, n_dot_o);
        bxdfSample.PDF_Specular = bxdfSample.PDF;
        bxdfSample.f_Specular   = bxdfSample.f;
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
#ifdef SAMPLE_VISIBLE_NORMALS_ENABLED
            vec3 w_hs = Sample_w_h_GGXVNDF(w_os, alpha, u.xy);
#else
            vec3 w_hs = Sample_w_h_GGX(w_os, alphaSqrd, u.xy);
#endif
            vec3 w_is = reflect(-w_os, w_hs);

            bxdfSample.w_iw = surfaceToWorld * w_is;

            //Prepare some dot products
            float n_dot_i = max(0.001f, w_is.z);
            float n_dot_h = max(0.001f, w_hs.z);
            float o_dot_h = max(0.001f, dot(w_os, w_hs));

            //Calculate Fresnel
            vec3 F = Fresnel(F0, n_dot_i);

            //Eval common BRDF
#ifdef SAMPLE_VISIBLE_NORMALS_ENABLED
            SBxDFEval specularBRDFEval  = Eval_f_Specular_GGXVNDF_Sampled(n_dot_i, n_dot_h, o_dot_h, n_dot_o, albedo, roughness, alphaSqrd, F);
#else
            SBxDFEval specularBRDFEval  = Eval_f_Specular_GGX_Sampled(n_dot_i, n_dot_h, o_dot_h, n_dot_o, albedo, roughness, alphaSqrd, F);
#endif
            bxdfSample.PDF          = specularBRDFEval.PDF;
            bxdfSample.f            = specularBRDFEval.f;
            bxdfSample.PDF_Specular = specularBRDFEval.PDF;
            bxdfSample.f_Specular   = specularBRDFEval.f;
        }
        else
        {     
            //Sample Microfacet Normal using GGX Distribution
            {
#ifdef SAMPLE_VISIBLE_NORMALS_ENABLED
                vec3 w_hs = Sample_w_h_GGXVNDF(w_os, alpha, u.xy);
#else
                vec3 w_hs = Sample_w_h_GGX(w_os, alphaSqrd, u.xy);
#endif
                vec3 w_is = reflect(-w_os, w_hs);

                bxdfSample.w_iw = surfaceToWorld * w_is;

                //Prepare some dot products
                float n_dot_i = max(0.001f, w_is.z);
                float n_dot_h = max(0.001f, w_hs.z);
                float o_dot_h = max(0.001f, dot(w_os, w_hs));

                //Calculate Fresnel
                vec3 F = Fresnel(F0, n_dot_i);

                //Eval common BRDF
#ifdef SAMPLE_VISIBLE_NORMALS_ENABLED
                SBxDFEval specularBRDFEval  = Eval_f_Specular_GGXVNDF_Sampled(n_dot_i, n_dot_h, o_dot_h, n_dot_o, albedo, roughness, alphaSqrd, F);
#else
                SBxDFEval specularBRDFEval  = Eval_f_Specular_GGX_Sampled(n_dot_i, n_dot_h, o_dot_h, n_dot_o, albedo, roughness, alphaSqrd, F);
#endif

                vec3 k_s = F;
                vec3 k_d = (1.0f - k_s) * (1.0f - metallic);            

                //Eval BRDF
                SBxDFEval lambertBRDFEval   = Eval_f_Diffuse(albedo, k_d, n_dot_i);    

                bxdfSample.PDF_Specular     = specularBRDFEval.PDF;
                bxdfSample.f_Specular       = specularBRDFEval.f;
                bxdfSample.PDF              = (lambertBRDFEval.PDF + specularBRDFEval.PDF) * 0.5f;
                bxdfSample.f                = lambertBRDFEval.f + specularBRDFEval.f;
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
#ifdef SAMPLE_VISIBLE_NORMALS_ENABLED
                SBxDFEval specularBRDFEval  = Eval_f_Specular_GGXVNDF_Sampled(n_dot_i, n_dot_h, o_dot_h, n_dot_o, albedo, roughness, alphaSqrd, F);
#else
                SBxDFEval specularBRDFEval  = Eval_f_Specular_GGX_Sampled(n_dot_i, n_dot_h, o_dot_h, n_dot_o, albedo, roughness, alphaSqrd, F);
#endif

                vec3 k_s = F;
                vec3 k_d = (1.0f - k_s) * (1.0f - metallic);            

                //Eval BRDF
                SBxDFEval lambertBRDFEval   = Eval_f_Diffuse(albedo, k_d, n_dot_i);    

                vec3 f      = (lambertBRDFEval.f + specularBRDFEval.f);
                float PDF   = (lambertBRDFEval.PDF + specularBRDFEval.PDF) * 0.5f;

                //We're taking 2 samples so we need to divide by 2 (multiply by 0.5)
                bxdfSample.f                = ((f * bxdfSample.PDF) + (bxdfSample.f * PDF)) * 0.5f;
                bxdfSample.PDF              = PDF * bxdfSample.PDF;
            }
        }
    }

    return bxdfSample;
}

SBxDFEval Eval_f(vec3 w_ow, vec3 w_nw, vec3 w_iw, vec3 albedo, float roughness, float metallic, vec3 F0)
{
    SBxDFEval bxdfEval;
    bxdfEval.f                = vec3(1.0f);
    bxdfEval.PDF              = 1.0f;

    float n_dot_o = dot(w_nw, w_ow);
    float n_dot_i = dot(w_nw, w_iw);

    //Perfect Specular are delta distributions -> (mathematically) zero percent chance that w_i gives a value of f > 0 
    if (roughness == 0.0f || n_dot_o <= 0.0f || n_dot_i <= 0.0f) return bxdfEval;

    float alpha             = roughness * roughness;
    float alphaSqrd         = max(alpha * alpha, 0.0000001f);

    vec3 w_hw = normalize(w_ow + w_iw);

    //Prepare some dot products
    n_dot_o = max(0.001f, n_dot_o);
    n_dot_i = max(0.001f, n_dot_i);
    float n_dot_h = max(0.001f, dot(w_nw, w_hw));
    float o_dot_h = max(0.001f, dot(w_ow, w_hw));

    //Calculate Fresnel
    vec3 F = Fresnel(F0, n_dot_i);

    //True metallic objects don't use a lambertian BRDF
    if (metallic == 1.0f)
    {
        //Eval Specular BRDF
#ifdef SAMPLE_VISIBLE_NORMALS_ENABLED
        SBxDFEval specularBRDFEval  = Eval_f_Specular_GGXVNDF_Sampled(n_dot_i, n_dot_h, o_dot_h, n_dot_o, albedo, roughness, alphaSqrd, F);
#else
        SBxDFEval specularBRDFEval  = Eval_f_Specular_GGX_Sampled(n_dot_i, n_dot_h, o_dot_h, n_dot_o, albedo, roughness, alphaSqrd, F);
#endif
        bxdfEval.PDF          = specularBRDFEval.PDF;
        bxdfEval.f            = specularBRDFEval.f;
    }
    else
    {     
        //Eval Specular BRDF
#ifdef SAMPLE_VISIBLE_NORMALS_ENABLED
        SBxDFEval specularBRDFEval  = Eval_f_Specular_GGXVNDF_Sampled(n_dot_i, n_dot_h, o_dot_h, n_dot_o, albedo, roughness, alphaSqrd, F);
#else
        SBxDFEval specularBRDFEval  = Eval_f_Specular_GGX_Sampled(n_dot_i, n_dot_h, o_dot_h, n_dot_o, albedo, roughness, alphaSqrd, F);
#endif

        vec3 k_s = F;
        vec3 k_d = (1.0f - k_s) * (1.0f - metallic);            

        //Eval Lambert BRDF
        SBxDFEval lambertBRDFEval   = Eval_f_Diffuse(albedo, k_d, n_dot_i);    

        bxdfEval.PDF              = (lambertBRDFEval.PDF + specularBRDFEval.PDF) * 0.5f;
        bxdfEval.f                = (lambertBRDFEval.f + specularBRDFEval.f);
    }

    return bxdfEval;
}