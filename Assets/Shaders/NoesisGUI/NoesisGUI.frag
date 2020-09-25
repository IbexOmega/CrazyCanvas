#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shader_draw_parameters : enable

#if defined(EFFECT_RGBA)

#elif defined(EFFECT_MASK)

#elif defined(EFFECT_PATH_SOLID)
    #define EFFECT_PATH 1
    #define PAINT_SOLID 1
    #define HAS_COLOR 1

#elif defined(EFFECT_PATH_LINEAR)
    #define EFFECT_PATH 1
    #define PAINT_LINEAR 1
    #define HAS_UV0 1

#elif defined(EFFECT_PATH_RADIAL)
    #define EFFECT_PATH 1
    #define PAINT_RADIAL 1
    #define HAS_UV0 1

#elif defined(EFFECT_PATH_PATTERN)
    #define EFFECT_PATH 1
    #define PAINT_PATTERN 1
    #define HAS_UV0 1

#elif defined(EFFECT_PATH_AA_SOLID)
    #define EFFECT_PATH_AA 1
    #define PAINT_SOLID 1
    #define HAS_COLOR 1
    #define HAS_COVERAGE 1

#elif defined(EFFECT_PATH_AA_LINEAR)
    #define EFFECT_PATH_AA 1
    #define PAINT_LINEAR 1
    #define HAS_COVERAGE 1
    #define HAS_UV0 1

#elif defined(EFFECT_PATH_AA_RADIAL)
    #define EFFECT_PATH_AA 1
    #define PAINT_RADIAL 1
    #define HAS_COVERAGE 1
    #define HAS_UV0 1

#elif defined(EFFECT_PATH_AA_PATTERN)
    #define EFFECT_PATH_AA 1
    #define PAINT_PATTERN 1
    #define HAS_COVERAGE 1
    #define HAS_UV0 1

#elif defined(EFFECT_SDF_SOLID)
    #define EFFECT_SDF 1
    #define PAINT_SOLID 1
    #define HAS_COLOR 1
    #define HAS_UV1 1
    #define HAS_ST1 1

#elif defined(EFFECT_SDF_LINEAR)
    #define EFFECT_SDF 1
    #define PAINT_LINEAR 1
    #define HAS_UV0 1
    #define HAS_UV1 1
    #define HAS_ST1 1

#elif defined(EFFECT_SDF_RADIAL)
    #define EFFECT_SDF 1
    #define PAINT_RADIAL 1
    #define HAS_UV0 1
    #define HAS_UV1 1
    #define HAS_ST1 1

#elif defined(EFFECT_SDF_PATTERN)
    #define EFFECT_SDF 1
    #define PAINT_PATTERN 1
    #define HAS_UV0 1
    #define HAS_UV1 1
    #define HAS_ST1 1

#elif defined(EFFECT_IMAGE_OPACITY_SOLID)
    #define EFFECT_IMAGE_OPACITY 1
    #define PAINT_SOLID 1
    #define HAS_COLOR 1
    #define HAS_UV1 1

#elif defined(EFFECT_IMAGE_OPACITY_LINEAR)
    #define EFFECT_IMAGE_OPACITY 1
    #define PAINT_LINEAR 1
    #define HAS_UV0 1
    #define HAS_UV1 1

#elif defined(EFFECT_IMAGE_OPACITY_RADIAL)
    #define EFFECT_IMAGE_OPACITY 1
    #define PAINT_RADIAL 1
    #define HAS_UV0 1
    #define HAS_UV1 1

#elif defined(EFFECT_IMAGE_OPACITY_PATTERN)
    #define EFFECT_IMAGE_OPACITY 1
    #define PAINT_PATTERN 1
    #define HAS_UV0 1
    #define HAS_UV1 1

#elif defined(EFFECT_IMAGE_SHADOW_35V)
    #define EFFECT_IMAGE_SHADOW_V 1
    #define GAUSSIAN_35_TAP 1
    #define HAS_COLOR 1
    #define HAS_UV1 1
    #define HAS_UV2 2

#elif defined(EFFECT_IMAGE_SHADOW_63V)
    #define EFFECT_IMAGE_SHADOW_V 1
    #define GAUSSIAN_63_TAP 1
    #define HAS_COLOR 1
    #define HAS_UV1 1
    #define HAS_UV2 1

#elif defined(EFFECT_IMAGE_SHADOW_127V)
    #define EFFECT_IMAGE_SHADOW_V 1
    #define GAUSSIAN_127_TAP 1
    #define HAS_COLOR 1
    #define HAS_UV1 1
    #define HAS_UV2 1

#elif defined(EFFECT_IMAGE_SHADOW_35H_SOLID)
    #define EFFECT_IMAGE_SHADOW_H 1
    #define GAUSSIAN_35_TAP 1
    #define PAINT_SOLID 1
    #define HAS_COLOR 1
    #define HAS_UV1 1
    #define HAS_UV2 2

#elif defined(EFFECT_IMAGE_SHADOW_35H_LINEAR)
    #define EFFECT_IMAGE_SHADOW_H 1
    #define GAUSSIAN_35_TAP 1
    #define PAINT_LINEAR 1
    #define HAS_UV0 1
    #define HAS_UV1 1
    #define HAS_UV2 2

#elif defined(EFFECT_IMAGE_SHADOW_35H_RADIAL)
    #define EFFECT_IMAGE_SHADOW_H 1
    #define GAUSSIAN_35_TAP 1
    #define PAINT_RADIAL 1
    #define HAS_UV0 1
    #define HAS_UV1 1
    #define HAS_UV2 2

#elif defined(EFFECT_IMAGE_SHADOW_35H_PATTERN)
    #define EFFECT_IMAGE_SHADOW_H 1
    #define GAUSSIAN_35_TAP 1
    #define PAINT_PATTERN 1
    #define HAS_UV0 1
    #define HAS_UV1 1
    #define HAS_UV2 2

#elif defined(EFFECT_IMAGE_SHADOW_63H_SOLID)
    #define EFFECT_IMAGE_SHADOW_H 1
    #define GAUSSIAN_63_TAP 1
    #define PAINT_SOLID 1
    #define HAS_COLOR 1
    #define HAS_UV1 1
    #define HAS_UV2 2

#elif defined(EFFECT_IMAGE_SHADOW_63H_LINEAR)
    #define EFFECT_IMAGE_SHADOW_H 1
    #define GAUSSIAN_63_TAP 1
    #define PAINT_LINEAR 1
    #define HAS_UV0 1
    #define HAS_UV1 1
    #define HAS_UV2 2

#elif defined(EFFECT_IMAGE_SHADOW_63H_RADIAL)
    #define EFFECT_IMAGE_SHADOW_H 1
    #define GAUSSIAN_63_TAP 1
    #define PAINT_RADIAL 1
    #define HAS_UV0 1
    #define HAS_UV1 1
    #define HAS_UV2 2

#elif defined(EFFECT_IMAGE_SHADOW_63H_PATTERN)
    #define EFFECT_IMAGE_SHADOW_H 1
    #define GAUSSIAN_63_TAP 1
    #define PAINT_PATTERN 1
    #define HAS_UV0 1
    #define HAS_UV1 1
    #define HAS_UV2 2

#elif defined(EFFECT_IMAGE_SHADOW_127H_SOLID)
    #define EFFECT_IMAGE_SHADOW_H 1
    #define GAUSSIAN_127_TAP 1
    #define PAINT_SOLID 1
    #define HAS_COLOR 1
    #define HAS_UV1 1
    #define HAS_UV2 2

#elif defined(EFFECT_IMAGE_SHADOW_127H_LINEAR)
    #define EFFECT_IMAGE_SHADOW_H 1
    #define GAUSSIAN_127_TAP 1
    #define PAINT_LINEAR 1
    #define HAS_UV0 1
    #define HAS_UV1 1
    #define HAS_UV2 2

#elif defined(EFFECT_IMAGE_SHADOW_127H_RADIAL)
    #define EFFECT_IMAGE_SHADOW_H 1
    #define GAUSSIAN_127_TAP 1
    #define PAINT_RADIAL 1
    #define HAS_UV0 1
    #define HAS_UV1 1
    #define HAS_UV2 2

#elif defined(EFFECT_IMAGE_SHADOW_127H_PATTERN)
    #define EFFECT_IMAGE_SHADOW_H 1
    #define GAUSSIAN_127_TAP 1
    #define PAINT_PATTERN 1
    #define HAS_UV0 1
    #define HAS_UV1 1
    #define HAS_UV2 2

#elif defined(EFFECT_IMAGE_BLUR_35V)
    #define EFFECT_IMAGE_BLUR_V 1
    #define GAUSSIAN_35_TAP 1
    #define HAS_COLOR 1
    #define HAS_UV1 1
    #define HAS_UV2 2

#elif defined(EFFECT_IMAGE_BLUR_63V)
    #define EFFECT_IMAGE_BLUR_V 1
    #define GAUSSIAN_63_TAP 1
    #define HAS_COLOR 1
    #define HAS_UV1 1
    #define HAS_UV2 1

#elif defined(EFFECT_IMAGE_BLUR_127V)
    #define EFFECT_IMAGE_BLUR_V 1
    #define GAUSSIAN_127_TAP 1
    #define HAS_COLOR 1
    #define HAS_UV1 1
    #define HAS_UV2 1

#elif defined(EFFECT_IMAGE_BLUR_35H_SOLID)
    #define EFFECT_IMAGE_BLUR_H 1
    #define GAUSSIAN_35_TAP 1
    #define PAINT_SOLID 1
    #define HAS_COLOR 1
    #define HAS_UV1 1
    #define HAS_UV2 2

#elif defined(EFFECT_IMAGE_BLUR_35H_LINEAR)
    #define EFFECT_IMAGE_BLUR_H 1
    #define GAUSSIAN_35_TAP 1
    #define PAINT_LINEAR 1
    #define HAS_UV0 1
    #define HAS_UV1 1
    #define HAS_UV2 2

#elif defined(EFFECT_IMAGE_BLUR_35H_RADIAL)
    #define EFFECT_IMAGE_BLUR_H 1
    #define GAUSSIAN_35_TAP 1
    #define PAINT_RADIAL 1
    #define HAS_UV0 1
    #define HAS_UV1 1
    #define HAS_UV2 2

#elif defined(EFFECT_IMAGE_BLUR_35H_PATTERN)
    #define EFFECT_IMAGE_BLUR_H 1
    #define GAUSSIAN_35_TAP 1
    #define PAINT_PATTERN 1
    #define HAS_UV0 1
    #define HAS_UV1 1
    #define HAS_UV2 2

#elif defined(EFFECT_IMAGE_BLUR_63H_SOLID)
    #define EFFECT_IMAGE_BLUR_H 1
    #define GAUSSIAN_63_TAP 1
    #define PAINT_SOLID 1
    #define HAS_COLOR 1
    #define HAS_UV1 1
    #define HAS_UV2 2

#elif defined(EFFECT_IMAGE_BLUR_63H_LINEAR)
    #define EFFECT_IMAGE_BLUR_H 1
    #define GAUSSIAN_63_TAP 1
    #define PAINT_LINEAR 1
    #define HAS_UV0 1
    #define HAS_UV1 1
    #define HAS_UV2 2

#elif defined(EFFECT_IMAGE_BLUR_63H_RADIAL)
    #define EFFECT_IMAGE_BLUR_H 1
    #define GAUSSIAN_63_TAP 1
    #define PAINT_RADIAL 1
    #define HAS_UV0 1
    #define HAS_UV1 1
    #define HAS_UV2 2

#elif defined(EFFECT_IMAGE_BLUR_63H_PATTERN)
    #define EFFECT_IMAGE_BLUR_H 1
    #define GAUSSIAN_63_TAP 1
    #define PAINT_PATTERN 1
    #define HAS_UV0 1
    #define HAS_UV1 1
    #define HAS_UV2 2

#elif defined(EFFECT_IMAGE_BLUR_127H_SOLID)
    #define EFFECT_IMAGE_BLUR_H 1
    #define GAUSSIAN_127_TAP 1
    #define PAINT_SOLID 1
    #define HAS_COLOR 1
    #define HAS_UV1 1
    #define HAS_UV2 2

#elif defined(EFFECT_IMAGE_BLUR_127H_LINEAR)
    #define EFFECT_IMAGE_BLUR_H 1
    #define GAUSSIAN_127_TAP 1
    #define PAINT_LINEAR 1
    #define HAS_UV0 1
    #define HAS_UV1 1
    #define HAS_UV2 2

#elif defined(EFFECT_IMAGE_BLUR_127H_RADIAL)
    #define EFFECT_IMAGE_BLUR_H 1
    #define GAUSSIAN_127_TAP 1
    #define PAINT_RADIAL 1
    #define HAS_UV0 1
    #define HAS_UV1 1
    #define HAS_UV2 2

#elif defined(EFFECT_IMAGE_BLUR_127H_PATTERN)
    #define EFFECT_IMAGE_BLUR_H 1
    #define GAUSSIAN_127_TAP 1
    #define PAINT_PATTERN 1
    #define HAS_UV0 1
    #define HAS_UV1 1
    #define HAS_UV2 2

#endif

////////////////////////////////////////////////////////////////////////////////////////////////////
#define SDF_SCALE 7.96875
#define SDF_BIAS 0.50196078431
#define SDF_AA_FACTOR 0.65
#define SDF_BASE_MIN 0.125
#define SDF_BASE_MAX 0.25
#define SDF_BASE_DEV -0.65

uniform vec4 rgba;
uniform float opacity;
uniform vec4 radialGrad[2];
uniform vec2 textPixelSize;
uniform float effectParams[32];

uniform sampler2D pattern;
uniform sampler2D ramps;
uniform sampler2D image;
uniform sampler2D glyphs;
uniform sampler2D shadow;

out vec4 fragColor;

#ifdef HAS_COLOR
    in vec4 color;
#endif

#ifdef HAS_UV0
    in vec2 uv0;
#endif

#ifdef HAS_UV1
    in vec2 uv1;
#endif

#ifdef HAS_UV2
    in vec4 uv2;
#endif

#ifdef HAS_ST1
    in vec2 st1;
#endif

#ifdef HAS_COVERAGE
    in float coverage;
#endif

#if defined(GAUSSIAN_35_TAP)
    #define GAUSSIAN_NUM_SAMPLES 9
    const float w[GAUSSIAN_NUM_SAMPLES] = float[]
    (
        0.10855, 0.13135, 0.10406, 0.07216, 0.04380, 0.02328, 0.01083, 0.00441, 0.00157
    );
    const float o[GAUSSIAN_NUM_SAMPLES] = float[]
    (
        0.66293, 2.47904, 4.46232, 6.44568, 8.42917, 10.41281, 12.39664, 14.38070, 16.36501
    );
#endif

#if defined(GAUSSIAN_63_TAP)
    #define GAUSSIAN_NUM_SAMPLES 16
    const float w[GAUSSIAN_NUM_SAMPLES] = float[]
    (
        0.05991, 0.07758, 0.07232, 0.06476, 0.05571, 0.04604, 0.03655, 0.02788, 0.02042, 0.01438,
        0.00972, 0.00631, 0.00394, 0.00236, 0.00136, 0.00075
    );
    const float o[GAUSSIAN_NUM_SAMPLES] = float[]
    (
        0.66555, 2.49371, 4.48868, 6.48366, 8.47864, 10.47362, 12.46860, 14.46360, 16.45860, 18.45361,
        20.44863, 22.44365, 24.43869, 26.43375, 28.42881, 30.42389
    );
#endif

#if defined(GAUSSIAN_127_TAP)
    #define GAUSSIAN_NUM_SAMPLES 32
    const float w[GAUSSIAN_NUM_SAMPLES] = float[]
    (
        0.02954, 0.03910, 0.03844, 0.03743, 0.03609, 0.03446, 0.03259, 0.03052, 0.02830, 0.02600,
        0.02365, 0.02130, 0.01900, 0.01679, 0.01469, 0.01272, 0.01092, 0.00928, 0.00781, 0.00651,
        0.00537, 0.00439, 0.00355, 0.00285, 0.00226, 0.00178, 0.00138, 0.00107, 0.00081, 0.00062,
        0.00046, 0.00034
    );
    const float o[GAUSSIAN_NUM_SAMPLES] = float[]
    (
        0.66640, 2.49848, 4.49726, 6.49605, 8.49483, 10.49362, 12.49240, 14.49119, 16.48997, 18.48876,
        20.48754, 22.48633, 24.48511, 26.48390, 28.48268, 30.48147, 32.48026, 34.47904, 36.47783, 38.47662,
        40.47540, 42.47419, 44.47298, 46.47176, 48.47055, 50.46934, 52.46813, 54.46692, 56.46571, 58.46450,
        60.46329, 62.46208
    );
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////
void main()
{
    /////////////////////////////////////////////////////
    // Fetch paint color and opacity
    /////////////////////////////////////////////////////
    #if defined(PAINT_SOLID)
        vec4 paint = color;
        float opacity_ = 1.0;

    #elif defined(PAINT_LINEAR)
        vec4 paint = texture2D(ramps, uv0);
        float opacity_ = opacity;

    #elif defined(PAINT_RADIAL)
        float dd = radialGrad[1].y * uv0.x - radialGrad[1].z * uv0.y;
        float u = radialGrad[0].x * uv0.x + radialGrad[0].y * uv0.y + radialGrad[0].z * 
            sqrt(uv0.x * uv0.x + uv0.y * uv0.y - dd * dd);
        vec4 paint = texture2D(ramps, vec2(u, radialGrad[1].w));
        float opacity_ = opacity;

    #elif defined(PAINT_PATTERN)
        vec4 paint = texture2D(pattern, uv0);
        float opacity_ = opacity;
    #endif

    /////////////////////////////////////////////////////
    // Apply selected effect
    /////////////////////////////////////////////////////
    #if defined(EFFECT_RGBA)
        fragColor = rgba;

    #elif defined(EFFECT_MASK)
        fragColor = vec4(1);

    #elif defined(EFFECT_PATH)
        fragColor = opacity_ * paint;

    #elif defined(EFFECT_PATH_AA)
        fragColor = (opacity_ * coverage) * paint;

    #elif defined(EFFECT_IMAGE_OPACITY)
        fragColor = texture2D(image, uv1) * (opacity_ * paint.a);

    #elif defined(EFFECT_IMAGE_SHADOW_V)
        #define BLUR_SIZE effectParams[0]

        float alpha = 0.0;
        float dir = BLUR_SIZE * textPixelSize.y;

        for (int i = 0; i < GAUSSIAN_NUM_SAMPLES; i++)
        {
            float offset = o[i] * dir;
            vec2 up = vec2(uv1.x, min(uv1.y + offset, uv2.w));
            vec2 down = vec2(uv1.x, max(uv1.y - offset, uv2.y));  
            alpha += w[i] * (texture2D(image, up).a + texture2D(image, down).a);
        }

        fragColor = vec4(0, 0, 0, alpha);

    #elif defined(EFFECT_IMAGE_SHADOW_H)
        #define SHADOW_COLOR vec4(effectParams[0], effectParams[1], effectParams[2], effectParams[3])
        #define BLUR_SIZE effectParams[4]
        #define SHADOW_OFFSETX effectParams[5]
        #define SHADOW_OFFSETY -effectParams[6]

        float alpha = 0.0;
        vec2 dir = vec2(BLUR_SIZE * textPixelSize.x, 0);
        vec2 offset = vec2(SHADOW_OFFSETX * textPixelSize.x, SHADOW_OFFSETY * textPixelSize.y);

        for (int i = 0; i < GAUSSIAN_NUM_SAMPLES; i++)
        {
            vec2 uvOffset = o[i] * dir;
            alpha += w[i] * (texture2D(shadow, clamp(uv1 - offset + uvOffset, uv2.xy, uv2.zw)).a + texture2D(shadow, clamp(uv1 - offset - uvOffset, uv2.xy, uv2.zw)).a);
        }

        vec4 img = texture2D(image, clamp(uv1, uv2.xy, uv2.zw));
        fragColor = (img + (1.0 - img.a) * (SHADOW_COLOR * alpha)) * (opacity_ * paint.a);

    #elif defined(EFFECT_IMAGE_BLUR_V)
        #define BLUR_SIZE effectParams[0]

        vec4 color = vec4(0);
        float dir = BLUR_SIZE * textPixelSize.y;

        for (int i = 0; i < GAUSSIAN_NUM_SAMPLES; i++)
        {
            float offset = o[i] * dir;
            vec2 up = vec2(uv1.x, min(uv1.y + offset, uv2.w));
            vec2 down = vec2(uv1.x, max(uv1.y - offset, uv2.y)); 
            color += w[i] * (texture2D(image, up) + texture2D(image, down));
        }

        fragColor = color;

    #elif defined(EFFECT_IMAGE_BLUR_H)
        #define BLUR_SIZE effectParams[0]

        vec4 color = vec4(0);
        float dir = BLUR_SIZE * textPixelSize.x;

        for (int i = 0; i < GAUSSIAN_NUM_SAMPLES; i++)
        {
            float offset = o[i] * dir;
            vec2 right = vec2(min(uv1.x + offset, uv2.z), uv1.y);
            vec2 left = vec2(max(uv1.x - offset, uv2.x), uv1.y);
            color += w[i] * (texture2D(image, right) + texture2D(image, left));
        }

        fragColor = color * (opacity_ * paint.a);

    #elif defined(EFFECT_SDF)
        vec4 color = texture2D(glyphs, uv1);
        float distance = SDF_SCALE * (color.r - SDF_BIAS);
        vec2 grad = dFdx(st1);

        float gradLen = length(grad);
        float scale = 1.0 / gradLen;
        float base = SDF_BASE_DEV * (1.0 - (clamp(scale, SDF_BASE_MIN, SDF_BASE_MAX) - SDF_BASE_MIN) / (SDF_BASE_MAX - SDF_BASE_MIN));
        float range = SDF_AA_FACTOR * gradLen;
        float alpha = smoothstep(base - range, base + range, distance);
        fragColor = (alpha * opacity_) * paint;

    #endif
}