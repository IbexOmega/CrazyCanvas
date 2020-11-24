#ifndef CUBEMAP_HELPER
#define CUBEMAP_HELPER

// Transform from dispatch ID to cubemap face direction
const mat3 ROTATE_UV[6] = 
{
	// +X
	mat3(  0,  0, -1,
		   0, -1,  0,
		   1,  0,  0),
	// -X
	mat3(  0,  0,  1,
		   0, -1,  0,
		  -1,  0,  0),
	// +Y
	mat3(  1,  0,  0,
		   0,  0,  1,
		   0,  1,  0),
	// -Y
	mat3(  1,  0,  0,
		   0,  0, -1,
		   0, -1,  0),
	// +Z
	mat3(  1,  0,  0,
		   0, -1,  0,
		   0,  0,  1),
	// -Z
	mat3( -1,  0,  0,
		   0, -1,  0,
		   0,  0, -1)
};

#endif