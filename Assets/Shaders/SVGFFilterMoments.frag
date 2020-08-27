#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable
//#extension GL_EXT_debug_printf : enable

#include "Helpers.glsl"
#include "Defines.glsl"
#include "SVGFCommon.glsl"

layout(location = 0) in vec2	in_TexCoord;

layout(binding = 0, set = NO_BUFFERS_TEXTURE_SET_INDEX) uniform sampler2D 	        u_DirectRadiance;
layout(binding = 1, set = NO_BUFFERS_TEXTURE_SET_INDEX) uniform sampler2D 	        u_IndirectRadiance;
layout(binding = 2, set = NO_BUFFERS_TEXTURE_SET_INDEX) uniform sampler2D 	        u_Moments;
layout(binding = 3, set = NO_BUFFERS_TEXTURE_SET_INDEX) uniform sampler2D 	        u_History;
layout(binding = 4, set = NO_BUFFERS_TEXTURE_SET_INDEX) uniform usampler2D 	        u_CompNormDepth;

layout(location = 0) out vec4	out_Direct;
layout(location = 1) out vec4	out_Indirect;

void main()
{
    const ivec2 iPos    = ivec2(gl_FragCoord.xy);
    float h             = max(1.0f, texelFetch(u_History, iPos, 0).r);
    ivec2 screenSize    = textureSize(u_DirectRadiance, 0);

    const vec4  directCenter        = texelFetch(u_DirectRadiance, iPos, 0);
    const vec4  indirectCenter      = texelFetch(u_IndirectRadiance, iPos, 0);

    if (h < 4.0f) // not enough temporal history available
    {
        float   sum_w_Direct    = 0.0f;
        float   sum_w_Indirect  = 0.0f;
        vec3    sum_Direct      = vec3(0.0f);
        vec3    sum_Indirect    = vec3(0.0f);
        vec4    sum_Moments     = vec4(0.0f);

		const float lDirectCenter       = luminance(directCenter.rgb);
        const float lIndirectCenter     = luminance(indirectCenter.rgb);

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
        
        const float phiLDirect   = PHI_COLOR;
        const float phiLIndirect = PHI_COLOR;
        const float phiDepth     = max(zCenter.y, 1e-8f) * 3.0f;
        const float phiNormal    = PHI_NORMAL;

        // compute first and second moment spatially. This code also applies cross-bilateral
        // filtering on the input color samples
        const int radius = 3;

        for (int yy = -radius; yy <= radius; yy++)
        {
            for (int xx = -radius; xx <= radius; xx++)
            {
                const ivec2 p           = iPos + ivec2(xx, yy);
                const bool inside       = all(greaterThanEqual(p, ivec2(0, 0))) && all(lessThan(p, screenSize));
                const bool samePixel    = (xx == 0) && (yy == 0);
                const float kernel      = 1.0f;

                if (inside)
                {
                    const vec3 directP     = texelFetch(u_DirectRadiance, p, 0).rgb;
                    const vec3 indirectP   = texelFetch(u_IndirectRadiance, p, 0).rgb;
                    const vec4 momentsP    = texelFetch(u_Moments, p, 0);

                    const float lDirectP   = luminance(directP.rgb);
                    const float lIndirectP = luminance(indirectP.rgb);

                    const uvec4 compNormDepthP = texelFetch(u_CompNormDepth, p, 0);
                    const vec3 normalP         = normalize(octToDir(compNormDepthP.x));
                    const vec2 zP              = vec2(uintBitsToFloat(compNormDepthP.y), uintBitsToFloat(compNormDepthP.z));

                    const vec2 w = computeWeight(
                        zCenter.x, zP.x, phiDepth * length(vec2(xx, yy)),
						normalCenter, normalP, phiNormal, 
                        lDirectCenter, lDirectP, phiLDirect,
                        lIndirectCenter, lIndirectP, phiLIndirect);

                    const float wDirect = w.x;
                    const float wIndirect = w.y;

                    sum_w_Direct    += wDirect;
                    sum_Direct      += directP * wDirect;

                    sum_w_Indirect  += wIndirect;
                    sum_Indirect    += indirectP * wIndirect;

					sum_Moments     += momentsP * vec4(wDirect.xx, wIndirect.xx);
                }
            }
        }

        // Clamp sums to >0 to avoid NaNs.
		sum_w_Direct      = max(sum_w_Direct, 1e-6f);
		sum_w_Indirect    = max(sum_w_Indirect, 1e-6f);

        sum_Direct   /= sum_w_Direct;
        sum_Indirect /= sum_w_Indirect;
        sum_Moments  /= vec4(sum_w_Direct.xx, sum_w_Indirect.xx);

        // compute variance for direct and indirect illumination using first and second moments
        vec2 variance = sum_Moments.ga - sum_Moments.rb * sum_Moments.rb;

        // give the variance a boost for the first frames
        variance *= 4.0 / h;

        //out_Direct     = vec4(sum_Direct,     variance.r);
        //out_Indirect   = vec4(sum_Indirect,   variance.g);

        out_Direct     = vec4(any(isnan(sum_Direct))    ? vec3(0.0f) : sum_Direct,     isnan(variance.r) ? 0.0f : variance.r);
        out_Indirect   = vec4(any(isnan(sum_Indirect))  ? vec3(0.0f) : sum_Indirect,   isnan(variance.g) ? 0.0f : variance.g);
    }
    else
    {
        out_Direct      = directCenter;
        out_Indirect    = indirectCenter;
    }
}