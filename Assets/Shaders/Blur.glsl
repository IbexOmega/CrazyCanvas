/*
*   This contains modified functions for bilateral blur based on the gaussian blur from this repo: https://github.com/Jam3/glsl-fast-gaussian-blur
*   This file must be included AFTER sampler2D binding declarations named:
*       - blurSrc - To be used as blur src
*       - gBufferAORoughMetalValid - To be sampled for roughness
*       - gBufferGeometricNormal - To be sampled for normals
*/

#include "Helpers.glsl"

vec4 blur5(in sampler2D blurSrc, vec2 uv, vec2 resolution, vec2 direction)
{
    vec2 off1 = vec2(1.3333333333333333) * direction;
    
    vec4 colorSum = vec4(0.0);
    float weightSum = 0.0f;
    vec2 uv_f;
    float weight;

    uv_f = uv;
    weight = 0.29411764705882354;
    colorSum += texture(blurSrc, uv_f) * weight;
    weightSum += weight;

    uv_f = uv + (off1 / resolution);
    weight = 0.35294117647058826;
    colorSum += texture(blurSrc, uv_f) * weight;
    weightSum += weight;

    uv_f = uv - (off1 / resolution);
    weight = 0.35294117647058826;
    colorSum += texture(blurSrc, uv_f) * weight;
    weightSum += weight;

    return colorSum / weightSum;
}

vec4 bilateralBlur5(in sampler2D blurSrc, in sampler2D gBufferAORoughMetalValid, in sampler2D gBufferGeometricNormal, vec2 uv, vec2 resolution, vec2 direction)
{
    vec2 off1 = vec2(1.3333333333333333) * direction;
    
    vec4 colorSum = vec4(0.0);
    float weightSum = 0.0f;
    vec2 uv_f;
    vec4 color;
    vec3 normal;
    float roughness;
    float weight;

    vec3 centerNormal = UnpackNormal(texture(gBufferGeometricNormal, uv).xyz);
    float centerRoughness = texture(gBufferAORoughMetalValid, uv).g;

    uv_f = uv;
    color = texture(blurSrc, uv_f);
    weight = color.a * 0.29411764705882354;
    colorSum += color * weight;
    weightSum += weight;

    uv_f = uv + (off1 / resolution);
    color = texture(blurSrc, uv_f);
    normal = UnpackNormal(texture(gBufferGeometricNormal, uv_f).xyz);
    roughness = texture(gBufferAORoughMetalValid, uv_f).g;
    weight = color.a * max(0.0f, dot(centerNormal, normal)) * (1.0f - abs(centerRoughness - roughness)) * 0.35294117647058826;
    colorSum += color * weight;
    weightSum += weight;

    uv_f = uv - (off1 / resolution);
    color = texture(blurSrc, uv_f);
    normal = UnpackNormal(texture(gBufferGeometricNormal, uv_f).xyz);
    roughness = texture(gBufferAORoughMetalValid, uv_f).g;
    weight = color.a * max(0.0f, dot(centerNormal, normal)) * (1.0f - abs(centerRoughness - roughness)) * 0.35294117647058826;
    colorSum += color * weight;
    weightSum += weight;

    return colorSum / weightSum;
}

vec4 blur9(in sampler2D blurSrc, vec2 uv, vec2 resolution, vec2 direction)
{
    vec2 off1 = vec2(1.3846153846) * direction;
    vec2 off2 = vec2(3.2307692308) * direction;

    vec4 colorSum = vec4(0.0);
    float weightSum = 0.0f;
    vec2 uv_f;
    float weight;

    uv_f = uv;
    weight = 0.2270270270;
    colorSum += texture(blurSrc, uv_f) * weight;
    weightSum += weight;

    uv_f = uv + (off1 / resolution);
    weight = 0.3162162162;
    colorSum += texture(blurSrc, uv_f) * weight;
    weightSum += weight;

    uv_f = uv - (off1 / resolution);
    weight = 0.3162162162;
    colorSum += texture(blurSrc, uv_f) * weight;
    weightSum += weight;

    uv_f = uv + (off2 / resolution);
    weight = 0.0702702703;
    colorSum += texture(blurSrc, uv_f) * weight;
    weightSum += weight;

    uv_f = uv - (off2 / resolution);
    weight = 0.0702702703;
    colorSum += texture(blurSrc, uv_f) * weight;
    weightSum += weight;

    return colorSum / weightSum;
}

vec4 bilateralBlur9(in sampler2D blurSrc, in sampler2D gBufferAORoughMetalValid, in sampler2D gBufferGeometricNormal, vec2 uv, vec2 resolution, vec2 direction)
{
    vec2 off1 = vec2(1.3846153846) * direction;
    vec2 off2 = vec2(3.2307692308) * direction;

    vec4 colorSum = vec4(0.0);
    float weightSum = 0.0f;
    vec2 uv_f;
    vec4 color;
    vec3 normal;
    float roughness;
    float weight;

    vec3 centerNormal = UnpackNormal(texture(gBufferGeometricNormal, uv).xyz);
    float centerRoughness = texture(gBufferAORoughMetalValid, uv).g;

    uv_f = uv;
    color = texture(blurSrc, uv_f);
    weight = color.a * 0.2270270270;
    colorSum += color * weight;
    weightSum += weight;

    uv_f = uv + (off1 / resolution);
    color = texture(blurSrc, uv_f);
    normal = UnpackNormal(texture(gBufferGeometricNormal, uv_f).xyz);
    roughness = texture(gBufferAORoughMetalValid, uv_f).g;
    weight = color.a * max(0.0f, dot(centerNormal, normal)) * (1.0f - abs(centerRoughness - roughness)) * 0.3162162162;
    colorSum += color * weight;
    weightSum += weight;

    uv_f = uv - (off1 / resolution);
    color = texture(blurSrc, uv_f);
    normal = UnpackNormal(texture(gBufferGeometricNormal, uv_f).xyz);
    roughness = texture(gBufferAORoughMetalValid, uv_f).g;
    weight = color.a * max(0.0f, dot(centerNormal, normal)) * (1.0f - abs(centerRoughness - roughness)) * 0.3162162162;
    colorSum += color * weight;
    weightSum += weight;

    uv_f = uv + (off2 / resolution);
    color = texture(blurSrc, uv_f);
    normal = UnpackNormal(texture(gBufferGeometricNormal, uv_f).xyz);
    roughness = texture(gBufferAORoughMetalValid, uv_f).g;
    weight = color.a * max(0.0f, dot(centerNormal, normal)) * (1.0f - abs(centerRoughness - roughness)) * 0.0702702703;
    colorSum += color * weight;
    weightSum += weight;

    uv_f = uv - (off2 / resolution);
    color = texture(blurSrc, uv_f);
    normal = UnpackNormal(texture(gBufferGeometricNormal, uv_f).xyz);
    roughness = texture(gBufferAORoughMetalValid, uv_f).g; 
    weight = color.a * max(0.0f, dot(centerNormal, normal)) * (1.0f - abs(centerRoughness - roughness)) * 0.0702702703;
    colorSum += color * weight;
    weightSum += weight;

    return colorSum / weightSum;
}

vec4 blur13(in sampler2D blurSrc, vec2 uv, vec2 resolution, vec2 direction)
{
    vec2 off1 = vec2(1.411764705882353) * direction;
    vec2 off2 = vec2(3.2941176470588234) * direction;
    vec2 off3 = vec2(5.176470588235294) * direction;

    vec4 colorSum = vec4(0.0);
    float weightSum = 0.0f;
    vec2 uv_f;
    float weight;

    uv_f = uv;
    weight = 0.1964825501511404;
    colorSum += texture(blurSrc, uv_f) * weight;
    weightSum += weight;

    uv_f = uv + (off1 / resolution); 
    weight = 0.2969069646728344;
    colorSum += texture(blurSrc, uv_f) * weight;
    weightSum += weight;

    uv_f = uv - (off1 / resolution); 
    weight = 0.2969069646728344;
    colorSum += texture(blurSrc, uv_f) * weight;
    weightSum += weight;

    uv_f = uv + (off2 / resolution); 
    weight = 0.09447039785044732;
    colorSum += texture(blurSrc, uv_f) * weight;
    weightSum += weight;

    uv_f = uv - (off2 / resolution); 
    weight = 0.09447039785044732;
    colorSum += texture(blurSrc, uv_f) * weight;
    weightSum += weight;

    uv_f = uv + (off3 / resolution); 
    weight = 0.010381362401148057;
    colorSum += texture(blurSrc, uv_f) * weight;
    weightSum += weight;

    uv_f = uv - (off3 / resolution); 
    weight = 0.010381362401148057;
    colorSum += texture(blurSrc, uv_f) * weight;
    weightSum += weight;

    return colorSum / weightSum;
}

vec4 bilateralBlur13(in sampler2D blurSrc, in sampler2D gBufferAORoughMetalValid, in sampler2D gBufferGeometricNormal, vec2 uv, vec2 resolution, vec2 direction)
{
    vec2 off1 = vec2(1.411764705882353) * direction;
    vec2 off2 = vec2(3.2941176470588234) * direction;
    vec2 off3 = vec2(5.176470588235294) * direction;

    vec4 colorSum = vec4(0.0);
    float weightSum = 0.0f;
    vec2 uv_f;
    vec4 color;
    vec3 normal;
    float roughness;
    float weight;

    vec3 centerNormal = UnpackNormal(texture(gBufferGeometricNormal, uv).xyz);
    float centerRoughness = texture(gBufferAORoughMetalValid, uv).g;

    uv_f = uv;
    color = texture(blurSrc, uv_f);
    weight = color.a * 0.1964825501511404;
    colorSum += color * weight;
    weightSum += weight;

    uv_f = uv + (off1 / resolution); 
    color = texture(blurSrc, uv_f);
    normal = UnpackNormal(texture(gBufferGeometricNormal, uv_f).xyz);
    roughness = texture(gBufferAORoughMetalValid, uv_f).g;
    weight = color.a * max(0.0f, dot(centerNormal, normal)) * (1.0f - abs(centerRoughness - roughness)) * 0.2969069646728344;
    colorSum += color * weight;
    weightSum += weight;

    uv_f = uv - (off1 / resolution); 
    color = texture(blurSrc, uv_f);
    normal = UnpackNormal(texture(gBufferGeometricNormal, uv_f).xyz);
    roughness = texture(gBufferAORoughMetalValid, uv_f).g;
    weight = color.a * max(0.0f, dot(centerNormal, normal)) * (1.0f - abs(centerRoughness - roughness)) * 0.2969069646728344;
    colorSum += color * weight;
    weightSum += weight;

    uv_f = uv + (off2 / resolution); 
    color = texture(blurSrc, uv_f);
    normal = UnpackNormal(texture(gBufferGeometricNormal, uv_f).xyz);
    roughness = texture(gBufferAORoughMetalValid, uv_f).g;
    weight = color.a * max(0.0f, dot(centerNormal, normal)) * (1.0f - abs(centerRoughness - roughness)) * 0.09447039785044732;
    colorSum += color * weight;
    weightSum += weight;

    uv_f = uv - (off2 / resolution); 
    color = texture(blurSrc, uv_f);
    normal = UnpackNormal(texture(gBufferGeometricNormal, uv_f).xyz);
    roughness = texture(gBufferAORoughMetalValid, uv_f).g;
    weight = color.a * max(0.0f, dot(centerNormal, normal)) * (1.0f - abs(centerRoughness - roughness)) * 0.09447039785044732;
    colorSum += color * weight;
    weightSum += weight;

    uv_f = uv + (off3 / resolution); 
    color = texture(blurSrc, uv_f);
    normal = UnpackNormal(texture(gBufferGeometricNormal, uv_f).xyz);
    roughness = texture(gBufferAORoughMetalValid, uv_f).g;
    weight = color.a * max(0.0f, dot(centerNormal, normal)) * (1.0f - abs(centerRoughness - roughness)) * 0.010381362401148057;
    colorSum += color * weight;
    weightSum += weight;

    uv_f = uv - (off3 / resolution); 
    color = texture(blurSrc, uv_f);
    normal = UnpackNormal(texture(gBufferGeometricNormal, uv_f).xyz);
    roughness = texture(gBufferAORoughMetalValid, uv_f).g;
    weight = color.a * max(0.0f, dot(centerNormal, normal)) * (1.0f - abs(centerRoughness - roughness)) * 0.010381362401148057;
    colorSum += color * weight;
    weightSum += weight;

    return colorSum / weightSum;
}