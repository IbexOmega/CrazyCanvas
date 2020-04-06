#pragma once

#ifdef UINT8_MAX
    #undef UINT8_MAX
#endif

#ifdef UINT16_MAX
    #undef UINT16_MAX
#endif

#ifdef UINT32_MAX
    #undef UINT32_MAX
#endif

#ifdef UINT64_MAX
    #undef UINT64_MAX
#endif

#ifdef INT8_MAX
    #undef INT8_MAX
#endif

#ifdef INT16_MAX
    #undef INT16_MAX
#endif

#ifdef INT32_MAX
    #undef INT32_MAX
#endif

#ifdef INT64_MAX
    #undef INT64_MAX
#endif

#ifdef INT8_MIN
    #undef INT8_MIN
#endif

#ifdef INT16_MIN
    #undef INT16_MIN
#endif

#ifdef INT32_MIN
    #undef INT32_MIN
#endif

#ifdef INT64_MIN
    #undef INT64_MIN
#endif

#ifdef FLT32_MAX
    #undef FLT32_MAX
#endif

#ifdef FLT32_MIN
    #undef FLT32_MIN
#endif

#ifdef FLT64_MAX
    #undef FLT64_MAX
#endif

#ifdef FLT64_MIN
    #undef FLT64_MIN
#endif

//Signed intergers
typedef char		int8;
typedef short		int16;
typedef int 		int32;
typedef long long	int64;

//Unsigned integers
typedef unsigned char		byte;
typedef unsigned char		uint8;
typedef unsigned short		uint16;
typedef unsigned int		uint32;
typedef unsigned long long	uint64;

//Floating point
typedef float	float32;
typedef double	float64;

//Other
typedef unsigned int GUID_Lambda;

//Max constants
constexpr uint8  UINT8_MAX  = 0xff;
constexpr uint16 UINT16_MAX = 0xffff;
constexpr uint32 UINT32_MAX = 0xffffffff;
constexpr uint64 UINT64_MAX = 0xffffffffffffffff;

constexpr int8  INT8_MIN  = int8(UINT8_MAX);
constexpr int8  INT8_MAX  = (INT8_MIN - 1);
constexpr int16 INT16_MAX = int16(UINT16_MAX);
constexpr int16 INT16_MIN = (INT16_MAX - 1);
constexpr int32 INT32_MAX = int32(UINT32_MAX);
constexpr int32 INT32_MIN = INT32_MAX - 1;
constexpr int64 INT64_MAX = int64(UINT64_MAX);
constexpr int64 INT64_MIN = (INT64_MAX - 1);

constexpr float32 FLT32_MAX = 3.402823466e+38f;
constexpr float32 FLT32_MIN = 1.175494351e-38f;
constexpr float64 FLT64_MAX = 1.7976931348623158e+308;
constexpr float64 FLT64_MIN = 2.2250738585072014e-308;
