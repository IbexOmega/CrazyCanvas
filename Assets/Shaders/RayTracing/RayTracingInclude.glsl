
const float RAY_NORMAL_OFFSET   = 0.01f;

struct SPrimaryPayload
{
    vec3    HitPosition;
    vec3    ShadingNormal;
    vec3    GeometricNormal;
    vec3    Albedo;
	float 	AO;
    float   Roughness;
    float   Metallic;
    float   Distance;
};