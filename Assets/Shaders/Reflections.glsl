#define GLOSSY_REFLECTIONS_ENABLED 1

#if GLOSSY_REFLECTIONS_ENABLED
    //Settings
    #define SAMPLE_VISIBLE_NORMALS_ENABLED      1
    #define SPATIAL_BRDF_DENOISING_ENABLED      1
    #define TEMPORAL_REUSE_ENABLED              1
    #define GAUSSIAN_FILTER_DENOISING_ENABLED   1
    #define BILATERAL_FILTER_DENOISING_ENABLED  1
    
    //Constants
	#define REFLECTION_REJECT_THRESHOLD         0.35f
    #define BRDF_TAIL_TRUNCATION_BIAS           0.85f //This is only used when using normal GGX sampling, when sampling the GGX ditribution of visible normals this is actually not necessary to avoid fireflies
    #define TEMPORAL_REUSE_MAX_HISTORY_LENGTH   16.0f
    #define BRDF_DENOISING_MIN_KERNEL_RADIUS    0
    #define BRDF_DENOISING_MAX_KERNEL_RADIUS    3
    #define GAUSSIAN_FILTER_LEVEL_0             0.01f
    #define GAUSSIAN_FILTER_LEVEL_1             0.05f
    #define GAUSSIAN_FILTER_LEVEL_2             0.1f
#else
    //Constants
	#define REFLECTION_REJECT_THRESHOLD         0.0f
    #define BRDF_TAIL_TRUNCATION_BIAS           0.0f
    #define TEMPORAL_REUSE_MAX_HISTORY_LENGTH   0.0f
    #define BRDF_DENOISING_MIN_KERNEL_RADIUS    0
    #define BRDF_DENOISING_MAX_KERNEL_RADIUS    0
#endif