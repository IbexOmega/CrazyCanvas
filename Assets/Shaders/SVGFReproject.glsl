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

layout(binding = 0, set = NO_BUFFERS_TEXTURE_SET_INDEX) uniform sampler2D 	        u_Motion;
layout(binding = 1, set = NO_BUFFERS_TEXTURE_SET_INDEX) uniform sampler2D 	        u_LinearZ;
layout(binding = 2, set = NO_BUFFERS_TEXTURE_SET_INDEX) uniform sampler2D 	        u_PrevDirectRadiance;
layout(binding = 3, set = NO_BUFFERS_TEXTURE_SET_INDEX) uniform sampler2D 	        u_PrevIndirectRadiance;
layout(binding = 4, set = NO_BUFFERS_TEXTURE_SET_INDEX) uniform sampler2D 	        u_PrevLinearZ;
layout(binding = 5, set = NO_BUFFERS_TEXTURE_SET_INDEX) uniform sampler2D 	        u_PrevMoments;
layout(binding = 6, set = NO_BUFFERS_TEXTURE_SET_INDEX, rgba32f) uniform image2D 	u_DirectRadiance;
layout(binding = 7, set = NO_BUFFERS_TEXTURE_SET_INDEX, rgba32f) uniform image2D 	u_IndirectRadiance;
layout(binding = 8, set = NO_BUFFERS_TEXTURE_SET_INDEX, r16f) uniform image2D 	    u_History;

layout(location = 0) out vec4	out_Moments;

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
    const vec2 imageDim = imageSize(u_DirectRadiance);
    const ivec2 iImageDim = ivec2(imageDim);
    
    // xy = motion, z = length(fwidth(pos)), w = length(fwidth(normal))
	vec4 motion = texelFetch(u_Motion, iPos, 0); 

    // +0.5 to account for texel center offset
    const ivec2 iPosPrev = ivec2(vec2(iPos) + motion.xy * imageDim + vec2(0.5f, 0.5f));
    const vec2 posPrev = floor(gl_FragCoord.xy) + motion.xy * imageDim;

    //Stores linear_Z, fwidth(linear_Z), Z_Prev, oct_Local_Normal
    vec4 depth = texelFetch(u_LinearZ, iPos, 0); 
    vec3 localNormal = octToDir(floatBitsToUint(depth.w));
    
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
    for (int sampleIndex = 0; sampleIndex < 4; sampleIndex)
    {
        ivec2 loc = ivec2(posPrev) + offset[sampleIndex];
        vec4 prevDepth = texelFetch(u_PrevLinearZ, loc, 0);
        vec3 prevLocalNormal = octToDir(floatBitsToUint(prevDepth.w));

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
                historyData.PrevDirect      += w[sampleIndex] * imageLoad(u_DirectRadiance, loc);
                historyData.PrevIndirect    += w[sampleIndex] * imageLoad(u_IndirectRadiance, loc);
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
                vec4 depthFilter = texelFetch(u_PrevLinearZ, p, 0);
				vec3 localNormalFilter = octToDir(floatBitsToUint(depthFilter.w));

                if (isReprojectionValid(iImageDim, iPosPrev, depth.z, depthFilter.x, depth.y, localNormal, localNormalFilter, motion.w))
                {
					historyData.PrevDirect   += imageLoad(u_DirectRadiance, p);
                    historyData.PrevIndirect += imageLoad(u_IndirectRadiance, p);
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
        historyData.HistoryLength = imageLoad(u_History, iPosPrev).r;
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
    vec3 direct         = imageLoad(u_DirectRadiance,   iPos).rgb;
    vec3 indirect       = imageLoad(u_IndirectRadiance, iPos).rgb;

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
    imageStore(u_DirectRadiance,    iPos, vec4(mix(historyData.PrevDirect,      vec4(direct,   0.0f),   alpha).rgb, variance.r));
    //imageStore(u_DirectRadiance,    iPos, vec4(1.0f, 0.0f, 0.0f, 1.0f));
    imageStore(u_IndirectRadiance,  iPos, vec4(mix(historyData.PrevIndirect,    vec4(indirect, 0.0f),   alpha).rgb, variance.g));
    imageStore(u_History,           iPos, vec4(historyData.HistoryLength));

    out_Moments = moments;
}