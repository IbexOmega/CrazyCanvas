#pragma once

// Signed Intergers
typedef char		int8;
typedef short		int16;
typedef int 		int32;
typedef long long	int64;

// Unsigned Integers
typedef unsigned char		byte;
typedef unsigned char		uint8;
typedef unsigned short		uint16;
typedef unsigned int		uint32;
typedef unsigned long long	uint64;

// Floating Point
typedef float	float32;
typedef double	float64;

// Other
typedef unsigned int GUID_Lambda;

// Disable the Macro redefinition warning for now
#ifdef LAMBDA_VISUAL_STUDIO
	#pragma warning(disable : 4005)
#endif

// Max constants
#ifndef UINT8_MAX
	#define UINT8_MAX 0xff
#endif

#ifndef UINT16_MAX
	#define UINT16_MAX 0xffff
#endif

#ifndef UINT32_MAX
	#define UINT32_MAX 0xffffffff
#endif

#ifndef UINT64_MAX
	#define UINT64_MAX 0xffffffffffffffff
#endif

#ifndef INT8_MAX
	#define INT8_MAX 127
#endif

#ifndef INT8_MIN
	#define INT8_MIN -128
#endif

#ifndef INT16_MAX
	#define INT16_MAX 32767
#endif

#ifndef INT16_MIN
	#define INT16_MIN -32768
#endif

#ifndef INT32_MAX
	#define INT32_MAX 0x7fffffff
#endif

#ifndef INT32_MIN
	#define INT32_MIN 0x80000000
#endif

#ifndef INT64_MAX
	#define INT64_MAX 0x7fffffffffffffff
#endif

#ifndef INT64_MIN
	#define INT64_MIN 0x8000000000000000
#endif

#ifndef FLT32_MAX
	#define FLT32_MAX 3.402823466e+38f
#endif

#ifndef FLT32_MIN
	#define FLT32_MIN 1.175494351e-38f
#endif

#ifndef FLT64_MAX
	#define FLT64_MAX 1.7976931348623158e+308
#endif

#ifndef FLT64_MIN
	#define FLT64_MIN 2.2250738585072014e-308
#endif
