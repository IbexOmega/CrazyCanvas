#version 460

#include "Helpers.glsl"

layout (local_size_x_id = 0, local_size_y = 1, local_size_z = 1) in;

layout(binding = 0, set = 0) uniform sampler2D u_InputImage;
layout(binding = 1, set = 0, rgba16f) uniform image2D u_OutputImage;
layout(binding = 2, set = 0) uniform sampler2D u_Albedo_AO;
layout(binding = 3, set = 0) uniform sampler2D u_Normal_Roughness;
layout(binding = 4, set = 0) uniform sampler2D u_Depth;

layout (push_constant) uniform PushConstants
{
	vec2 Direction;
    float MaxTemporalFrames;
    float MinTemporalWeight;
	float ReflectionRayBias;
	float ShadowRayBias;
} u_PushConstants;

void main()
{
    ivec2 BLUR_IMAGE_SIZE = imageSize(u_OutputImage);
    int BLUR_IMAGE_TOTAL_NUM_PIXELS = BLUR_IMAGE_SIZE.x * BLUR_IMAGE_SIZE.y;

    if (gl_GlobalInvocationID.x >= BLUR_IMAGE_TOTAL_NUM_PIXELS) 
        return;

    ivec2 dstPixelCoords = ivec2(gl_GlobalInvocationID.x % BLUR_IMAGE_SIZE.x, gl_GlobalInvocationID.x / BLUR_IMAGE_SIZE.x);
    vec2 texCoords = (vec2(dstPixelCoords) + 0.5f) / vec2(BLUR_IMAGE_SIZE);

    vec4 centerColor = texture(u_InputImage, texCoords);

    if (centerColor.a < u_PushConstants.MaxTemporalFrames)
    {
        //float roughness = texture(u_Normal_Roughness, texCoords).a;
        //float depth = texture(u_Depth, texCoords).r;

        vec2 imageResolution = textureSize(u_InputImage, 0);
        vec2 normalizedDirection = u_PushConstants.Direction / imageResolution;

        vec3 blurColor = bilateralBlur(u_InputImage, centerColor, texCoords, normalizedDirection).rgb;
        imageStore(u_OutputImage, dstPixelCoords, vec4(blurColor, (2.0f / (u_PushConstants.Direction.x + 1.0f)) * centerColor.a));
    }
    else
    {
        imageStore(u_OutputImage, dstPixelCoords, centerColor);
    }
}