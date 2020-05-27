#version 460
#extension GL_GOOGLE_include_directive : enable

#include "Helpers.glsl"

layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

layout(binding = 0, set = 0) uniform sampler2D u_ShadedImage;
layout(binding = 1, set = 0, rgba8) writeonly uniform image2D   u_OutputImage;

void main()
{
    ivec2 OUTPUT_IMAGE_SIZE = imageSize(u_OutputImage);
    int OUTPUT_IMAGE_TOTAL_NUM_PIXELS = OUTPUT_IMAGE_SIZE.x * OUTPUT_IMAGE_SIZE.y;

    if (gl_GlobalInvocationID.x >= OUTPUT_IMAGE_TOTAL_NUM_PIXELS) 
        return;

    ivec2 dstPixelCoords = ivec2(gl_GlobalInvocationID.x % OUTPUT_IMAGE_SIZE.x, gl_GlobalInvocationID.x / OUTPUT_IMAGE_SIZE.x);
    vec2 texCoords = (vec2(dstPixelCoords) + 0.5f) / vec2(OUTPUT_IMAGE_SIZE);

    vec4 centerColor = texture(u_ShadedImage, texCoords);

    imageStore(u_OutputImage, dstPixelCoords, centerColor);
}