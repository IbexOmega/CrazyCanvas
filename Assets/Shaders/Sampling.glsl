#include "Defines.glsl"

/*
    Non-Uniform Sample Generators
*/

float RadicalInverse_VdC(uint bits)
{
	bits = (bits << 16u) | (bits >> 16u);
	bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
	bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
	bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
	bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
	return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

vec2 Hammersley(uint i, uint N)
{
	return vec2(float(i) / float(N), RadicalInverse_VdC(i));
}

/*
    Shape Sample Functions
*/

float SphereSurfaceArea(float radius)
{
	return FOUR_PI * radius * radius;
}

float DiskSurfaceArea(float radius)
{
	return PI * radius * radius;
}

float QuadSurfaceArea(float radiusX, float radiusY)
{
	return 4 * radiusX * radiusY;
}

vec2 ConcentricSampleDisk(vec2 u)
{
	vec2 uOffset = u * 2.0f - 1.0f;
	if (dot(uOffset, uOffset) == 0.0f) return vec2(0.0f);

	float r     = 0.0f;
	float theta = 0.0f;

	if (abs(uOffset.x) > abs(uOffset.y))
	{
		r = uOffset.x;
		theta = PI_OVER_FOUR * uOffset.y / uOffset.x;
	}
	else
	{
		r = uOffset.y;
		theta = PI_OVER_TWO - PI_OVER_FOUR * uOffset.x / uOffset.y;
	}

	return r * vec2(cos(theta), sin(theta));
}

//These functions assume that the untransformed quad lies in the xz-plane with normal pointing in positive y direction and has a radius of 1 (side length of 2)
float QuadPDF(mat4 transform)
{
	return 1.0f / QuadSurfaceArea(length(transform[0].xyz), length(transform[2].xyz));
}

vec3 QuadNormal(mat4 transform)
{
	return (transform * vec4(0.0f, 1.0f, 0.0f, 0.0f)).xyz;
}

SShapeSample SampleQuad(mat4 transform, vec2 u)
{
	SShapeSample shapeSample;

	u = u * 2.0f - 1.0f;
	shapeSample.Position 	= (transform * vec4(u.x, 0.0f, u.y, 1.0f)).xyz; //Assume quad thickiness of 0.0f
	shapeSample.Normal		= QuadNormal(transform);
	shapeSample.PDF			= QuadPDF(transform);

	return shapeSample;
}

SShapeSample SampleQuad(vec3 position, vec3 direction, float radius, vec2 u)
{
	SShapeSample shapeSample;

	direction = normalize(direction);

	vec3 tangent;
	vec3 bitangent;
	CreateCoordinateSystem(direction, tangent, bitangent);

	u = u * 2.0f - 1.0f;
	shapeSample.Position 	= position + radius * (u.x * tangent + u.y * bitangent); //Assume quad thickiness of 0.0f
	shapeSample.Normal		= direction;
	shapeSample.PDF			= 1.0f / QuadSurfaceArea(radius, radius);

	return shapeSample;
}

SShapeSample SampleDisk(vec3 position, vec3 direction, float radius, vec2 u)
{
	SShapeSample shapeSample;

	direction = normalize(direction);

	vec3 tangent;
	vec3 bitangent;
	CreateCoordinateSystem(direction, tangent, bitangent);

	vec2 pd = ConcentricSampleDisk(u);
	shapeSample.Position 	= position + radius * (pd.x * tangent + pd.y * bitangent); //Assume disk thickiness of 0.0f
	shapeSample.Normal		= direction;
	shapeSample.PDF			= 1.0f / DiskSurfaceArea(radius);

	return shapeSample;
}

SShapeSample UniformSampleUnitSphere(vec2 u)
{
	SShapeSample shapeSample;

	float z 	= 1.0f - 2.0f * u.x;
	float r 	= sqrt(max(0.0f, 1.0f - z * z));
	float phi 	= 2.0f * PI * u.y;

	shapeSample.Position 	= vec3(r * cos(phi), r * sin(phi), z);
	shapeSample.Normal	 	= normalize(shapeSample.Position);
	shapeSample.PDF 		= 1.0f / FOUR_PI;
	return shapeSample;
}

SShapeSample UniformSampleSphereSurface(vec3 position, float radius, vec2 u)
{
	SShapeSample shapeSample = UniformSampleUnitSphere(u);

	shapeSample.Position	 = position + radius * shapeSample.Normal; //The normal as returned from UniformSampleUnitSphere can be interpreted as a point on the surface
	shapeSample.PDF 		 /= (radius * radius);
	return shapeSample;
}