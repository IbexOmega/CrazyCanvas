#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable
//#extension GL_EXT_debug_printf : enable

#include "Helpers.glsl"
#include "Defines.glsl"
#include "SVGFCommon.glsl"

layout(location = 0) in vec2	in_TexCoord;

layout(binding = 0, set = NO_BUFFERS_TEXTURE_SET_INDEX) uniform sampler2D 	u_DirectRadiance;
layout(binding = 1, set = NO_BUFFERS_TEXTURE_SET_INDEX) uniform sampler2D 	u_IndirectRadiance;
layout(binding = 2, set = NO_BUFFERS_TEXTURE_SET_INDEX) uniform usampler2D 	u_CompNormDepth;
layout(binding = 3, set = NO_BUFFERS_TEXTURE_SET_INDEX) uniform sampler2D 	u_History;

layout(location = 0) out vec4   out_Direct;
layout(location = 1) out vec4   out_Indirect;

// computes a 3x3 gaussian blur of the variance, centered around
// the current pixel
vec2 computeVarianceCenter(ivec2 iPos)
{
    vec2 sum = vec2(0.0f, 0.0f);

    const float kernel[2][2] = 
    {
        { 1.0f / 4.0f, 1.0f / 8.0f  },
        { 1.0f / 8.0f, 1.0f / 16.0f }
    };

    const int radius = 1;
    for (int yy = -radius; yy <= radius; yy++)
    {
        for (int xx = -radius; xx <= radius; xx++)
        {
            ivec2 p = iPos + ivec2(xx, yy);

            float k = kernel[abs(xx)][abs(yy)];

            sum.r += texelFetch(u_DirectRadiance, p, 0).a * k;
            sum.g += texelFetch(u_IndirectRadiance, p, 0).a * k;
        }
    }

    return sum;
}

void main()
{
    const float PHI_COLOR    = 10.0f;
    const float PHI_NORMAL   = 128.0f;
    const int   STEP_SIZE    = 2;

    const ivec2 iPos       = ivec2(gl_FragCoord.xy);
    const ivec2 screenSize = textureSize(u_DirectRadiance, 0);

    const float epsVariance      = 1e-10f;
    const float kernelWeights[3] = { 1.0f, 2.0f / 3.0f, 1.0f / 6.0f };

    // constant samplers to prevent the compiler from generating code which
    // fetches the sampler descriptor from memory for each texture access
    const vec4  directCenter    = texelFetch(u_DirectRadiance, iPos, 0);
    const vec4  indirectCenter  = texelFetch(u_IndirectRadiance, iPos, 0);
    const float lDirectCenter   = luminance(directCenter.rgb);
    const float lIndirectCenter = luminance(indirectCenter.rgb);

    // variance for direct and indirect, filtered using 3x3 gaussin blur
    const vec2 var = computeVarianceCenter(iPos);

    // number of temporally integrated pixels
    const float historyLength = texelFetch(u_History, iPos, 0).r;

    const uvec4 compNormDepthCenter = texelFetch(u_CompNormDepth, iPos, 0);
    const vec3 normalCenter         = normalize(octToDir(compNormDepthCenter.x));
    const vec2 zCenter              = vec2(uintBitsToFloat(compNormDepthCenter.y), uintBitsToFloat(compNormDepthCenter.z));

     if (zCenter.x < 0.0f)
    {
        // current pixel does not have a valid depth => must be envmap => do nothing
        out_Direct   = directCenter;
        out_Indirect = indirectCenter;
        return;
    }

    const float phiLDirect   = PHI_COLOR * sqrt(max(0.0, epsVariance + var.r));
    const float phiLIndirect = PHI_COLOR * sqrt(max(0.0, epsVariance + var.g));
    const float phiDepth     = max(zCenter.y, 1e-8f) * float(STEP_SIZE);

    // explicitly store/accumulate center pixel with weight 1 to prevent issues
    // with the edge-stopping functions
    float   sumWDirect      = 1.0f;
    float   sumWIndirect    = 1.0f;
    vec4    sumDirect        = directCenter;
    vec4    sumIndirect      = indirectCenter;

    for (int yy = -2; yy <= 2; yy++)
    {
        for (int xx = -2; xx <= 2; xx++)
        {
            const ivec2 p     = iPos + ivec2(xx, yy) * STEP_SIZE;
            const bool inside = all(greaterThanEqual(p, ivec2(0, 0))) && all(lessThan(p, screenSize));

            const float kernel = kernelWeights[abs(xx)] * kernelWeights[abs(yy)];

            if (inside && (xx != 0 || yy != 0)) // skip center pixel, it is already accumulated
            {
                const vec4 directP          = texelFetch(u_DirectRadiance, p, 0);
                const vec4 indirectP        = texelFetch(u_IndirectRadiance, p, 0);

                const uvec4 compNormDepthP  = texelFetch(u_CompNormDepth, p, 0);
                const vec3 normalP          = normalize(octToDir(compNormDepthP.x));
                const vec2 zP               = vec2(uintBitsToFloat(compNormDepthP.y), uintBitsToFloat(compNormDepthP.z));

                const float lDirectP        = luminance(directP.rgb);
                const float lIndirectP      = luminance(indirectP.rgb);

                // compute the edge-stopping functions
                const vec2 w = computeWeight(
                    zCenter.x, zP.x, phiDepth * length(vec2(xx, yy)),
					normalCenter, normalP, PHI_NORMAL, 
                    lDirectCenter, lDirectP, phiLDirect,
                    lIndirectCenter, lIndirectP, phiLIndirect);

                const float wDirect = w.x * kernel;
                const float wIndirect = w.y * kernel;

                // alpha channel contains the variance, therefore the weights need to be squared, see paper for the formula
                sumWDirect  += wDirect;
                sumDirect   += vec4(wDirect.xxx, wDirect * wDirect) * directP;

                sumWIndirect  += wIndirect;
                sumIndirect   += vec4(wIndirect.xxx, wIndirect * wIndirect) * indirectP;
            }
        }
    }

    sumWDirect      = max(0.001f, sumWDirect);
    sumWIndirect    = max(0.001f, sumWIndirect);

    // renormalization is different for variance, check paper for the formula
    out_Direct   = vec4(sumDirect   / vec4(sumWDirect.xxx,   sumWDirect   * sumWDirect  ));
    out_Indirect = vec4(sumIndirect / vec4(sumWIndirect.xxx, sumWIndirect * sumWIndirect));

    //out_Direct.rgb = any(isnan(out_Direct.rgb)) ? vec3(1.0f) : out_Direct.rgb;
    //out_Direct.a = isnan(out_Direct.a) ? 1.0f : out_Direct.a;
    //out_Indirect.rgb = any(isnan(out_Indirect.rgb)) ? vec3(1.0f) : out_Indirect.rgb;
    //out_Indirect.a = isnan(out_Indirect.a) ? 1.0f : out_Indirect.a;
}