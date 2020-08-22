#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable
//#extension GL_EXT_debug_printf : enable

#include "Helpers.glsl"
#include "Defines.glsl"

struct SHistoryData
{
    vec4 PrevDirect;
    vec4 PrevIndirect;
    vec4 PrevMoments;
    float HistoryLength;
    bool Valid;
};

layout(location = 0) in vec2	in_TexCoord;

layout(binding = 0, set = NO_BUFFERS_TEXTURE_SET_INDEX) uniform sampler2D 	        u_DirectRadiance;
layout(binding = 1, set = NO_BUFFERS_TEXTURE_SET_INDEX) uniform sampler2D 	        u_IndirectRadiance;
layout(binding = 2, set = NO_BUFFERS_TEXTURE_SET_INDEX) uniform sampler2D 	        u_History;
layout(binding = 3, set = NO_BUFFERS_TEXTURE_SET_INDEX) uniform sampler2D 	        u_Motion;
layout(binding = 4, set = NO_BUFFERS_TEXTURE_SET_INDEX) uniform usampler2D 	        u_LinearZ;
layout(binding = 5, set = NO_BUFFERS_TEXTURE_SET_INDEX) uniform sampler2D 	        u_PrevDirectRadiance;
layout(binding = 6, set = NO_BUFFERS_TEXTURE_SET_INDEX) uniform sampler2D 	        u_PrevIndirectRadiance;
layout(binding = 7, set = NO_BUFFERS_TEXTURE_SET_INDEX) uniform usampler2D 	        u_PrevLinearZ;
layout(binding = 8, set = NO_BUFFERS_TEXTURE_SET_INDEX) uniform sampler2D 	        u_PrevMoments;

layout(location = 0) out vec4	out_Direct;
layout(location = 1) out vec4	out_Indirect;
layout(location = 2) out vec4	out_Moments;
layout(location = 3) out float	out_History;

bool isReprojectionValid(ivec2 iImageDim, ivec2 coord, float Z, float Zprev, float fwidthZ, vec3 localNormal, vec3 prevLocalNormal, float fwidthNormal)
{
    // check whether reprojected pixel is inside of the screen
    if(any(lessThan(coord, ivec2(1, 1))) || any(greaterThan(coord, iImageDim - ivec2(1, 1)))) return false;
    // check if deviation of depths is acceptable
    if(abs(Zprev - Z) / (fwidthZ + 1e-4) > 2.0f) return false;
    // check normals for compatibility
    if(distance(localNormal, prevLocalNormal) / (fwidthNormal + 1e-2) > 16.0f) return false;

    return true;
}

SHistoryData LoadHistoryData()
{
    const ivec2 iPos = ivec2(gl_FragCoord.xy);
    const vec2 imageDim = textureSize(u_DirectRadiance, 0);
    const ivec2 iImageDim = ivec2(imageDim);
    
    // xy = motion, z = length(fwidth(pos)), w = length(fwidth(normal))
	vec4 motion = texelFetch(u_Motion, iPos, 0); 

    // +0.5 to account for texel center offset
    const ivec2 iPosPrev = ivec2(vec2(iPos) + motion.xy * imageDim + vec2(0.5f, 0.5f));
    const vec2 posPrev = floor(gl_FragCoord.xy) + motion.xy * imageDim;

    //Stores linear_Z, fwidth(linear_Z), Z_Prev, oct_Local_Normal
    uvec4 sampledDepth = texelFetch(u_LinearZ, iPos, 0); 
    vec3 depth = uintBitsToFloat(sampledDepth.xyz);
    vec3 localNormal = octToDir(sampledDepth.w);
    
    SHistoryData historyData;
    historyData.PrevDirect      = vec4(0.0f);
    historyData.PrevIndirect    = vec4(0.0f);
    historyData.PrevMoments     = vec4(0.0f);
    historyData.Valid           = false;

    bvec4 v;
    ivec2 offset[4] = 
    {
        ivec2(0, 0),
        ivec2(1, 0),
        ivec2(0, 1),
        ivec2(1, 1)
    };

    // check for all 4 taps of the bilinear filter for validity
    for (int sampleIndex = 0; sampleIndex < 4; sampleIndex++)
    {
        ivec2 loc = ivec2(posPrev) + offset[sampleIndex];
        uvec4 sampledPrevDepth = texelFetch(u_PrevLinearZ, loc, 0);
        vec3 prevDepth = uintBitsToFloat(sampledPrevDepth.xyz);
        vec3 prevLocalNormal = octToDir(sampledPrevDepth.w);

        v[sampleIndex] = isReprojectionValid(iImageDim, iPosPrev, depth.z, prevDepth.x, depth.y, localNormal, prevLocalNormal, motion.w);
        historyData.Valid = historyData.Valid || v[sampleIndex];
    }

    if (historyData.Valid)
    {
        float sum_w = 0.0f;
        float x = fract(posPrev.x);
        float y = fract(posPrev.y);

        // bilinear weights
        vec4 w = vec4((1.0f -   x) * (1.0f - y), 
                                x  * (1.0f - y), 
                        (1.0f - x) *         y,
                                x  *         y);

        // perform the actual bilinear interpolation
        for (int sampleIndex = 0; sampleIndex < 4; sampleIndex++)
        {
            ivec2 loc = ivec2(posPrev) + offset[sampleIndex];            
            if (v[sampleIndex])
            {
                historyData.PrevDirect      += w[sampleIndex] * texelFetch(u_DirectRadiance, loc, 0);
                historyData.PrevIndirect    += w[sampleIndex] * texelFetch(u_IndirectRadiance, loc, 0);
                historyData.PrevMoments     += w[sampleIndex] * texelFetch(u_PrevMoments, loc, 0);
                sum_w                       += w[sampleIndex];
            }
        }

        // redistribute weights in case not all taps were used
		historyData.Valid = (sum_w >= 0.01f);
		historyData.PrevDirect   = historyData.Valid ?  historyData.PrevDirect     / sum_w : vec4(0.0f);
		historyData.PrevIndirect = historyData.Valid ?  historyData.PrevIndirect   / sum_w : vec4(0.0f);
		historyData.PrevMoments  = historyData.Valid ?  historyData.PrevMoments    / sum_w : vec4(0.0f);               
    }
    else  // perform cross-bilateral filter in the hope to find some suitable samples somewhere
    {
        float count = 0.0f;

        // this code performs a binary descision for each tap of the cross-bilateral filter
        const int radius = 1;
        for (int yy = -radius; yy <= radius; yy++)
        {
            for (int xx = -radius; xx <= radius; xx++)
            {
                ivec2 p = iPosPrev + ivec2(xx, yy);
                uvec4 sampledDepthFilter = texelFetch(u_PrevLinearZ, p, 0);
                vec3 depthFilter = uintBitsToFloat(sampledDepthFilter.xyz);
				vec3 localNormalFilter = octToDir(sampledDepthFilter.w);

                if (isReprojectionValid(iImageDim, iPosPrev, depth.z, depthFilter.x, depth.y, localNormal, localNormalFilter, motion.w))
                {
					historyData.PrevDirect   += texelFetch(u_DirectRadiance, p, 0);
                    historyData.PrevIndirect += texelFetch(u_IndirectRadiance, p, 0);
					historyData.PrevMoments  += texelFetch(u_PrevMoments, p, 0);
                    count += 1.0f;
                }
            }
        }

        if (count > 0.0f)
        {
            historyData.Valid = true;
            historyData.PrevDirect   /= count;
            historyData.PrevIndirect /= count;
            historyData.PrevMoments  /= count;
        }
    }

    if (historyData.Valid)
    {
        historyData.HistoryLength = texelFetch(u_History, iPosPrev, 0).r;
    }
    else
    {
        historyData.PrevDirect      = vec4(0.0f);
        historyData.PrevIndirect    = vec4(0.0f);
        historyData.PrevMoments     = vec4(0.0f);
        historyData.HistoryLength   = 0.0f;
        historyData.Valid           = false;
    }

    return historyData;
}

void main()
{
    
    const ivec2 iPos    = ivec2(gl_FragCoord.xy);
    vec3 direct         = texelFetch(u_DirectRadiance,   iPos, 0).rgb;
    vec3 indirect       = texelFetch(u_IndirectRadiance, iPos, 0).rgb;

    SHistoryData historyData = LoadHistoryData();
    historyData.HistoryLength = min(32.0f, historyData.Valid ? historyData.HistoryLength + 1.0f : 1.0f);

    // this adjusts the alpha for the case where insufficient history is available.
    // It boosts the temporal accumulation to give the samples equal weights in the beginning.
    const float ALPHA = 0.2f;
    const float MOMENTS_ALPHA = 0.2f;
    const float alpha        = historyData.Valid ? max(ALPHA,           1.0f / historyData.HistoryLength) : 1.0f;
    const float alphaMoments = historyData.Valid ? max(MOMENTS_ALPHA,   1.0f / historyData.HistoryLength) : 1.0f;

    // compute first two moments of luminance
    vec4 moments;
    moments.r = luminance(direct);
    moments.b = luminance(indirect);
    moments.g = moments.r * moments.r;
    moments.a = moments.b * moments.b;

    // temporal integration of the moments
    moments = mix(historyData.PrevMoments, moments, alphaMoments);

    vec2 variance = max(vec2(0.0f), moments.ga - moments.rb * moments.rb);

    // temporal integration of direct and indirect illumination
    // variance is propagated through the alpha channel
    out_Direct      = vec4(mix(historyData.PrevDirect.rgb,      direct,   alpha), variance.r);
    out_Indirect    = vec4(mix(historyData.PrevIndirect.rgb,    indirect, alpha), variance.g);
    out_History     = historyData.HistoryLength;
    out_Moments     = moments;
}