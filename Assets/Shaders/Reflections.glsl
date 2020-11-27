#define GLOSSY_REFLECTIONS_ENABLED 1

#if GLOSSY_REFLECTIONS_ENABLED
    //Settings
    #define SPATIAL_BRDF_DENOISING_ENABLED      0
    #define TEMPORAL_REUSE_ENABLED              1

    //Constants
	#define REFLECTION_REJECT_THRESHOLD         0.35f
    #define BRDF_TAIL_TRUNCATION_BIAS           0.85f
    #define TEMPORAL_REUSE_MAX_HISTORY_LENGTH   32.0f
#else
    //Constants
	#define REFLECTION_REJECT_THRESHOLD         0.0f
    #define BRDF_TAIL_TRUNCATION_BIAS           0.0f
#endif