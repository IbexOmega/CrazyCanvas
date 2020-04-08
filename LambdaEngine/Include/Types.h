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

// Max constants
constexpr uint8  UINT8_MAX_  = 0xff;
constexpr uint16 UINT16_MAX_ = 0xffff;
constexpr uint32 UINT32_MAX_ = 0xffffffff;
constexpr uint64 UINT64_MAX_ = 0xffffffffffffffff;

constexpr int8  INT8_MAX_  = 127;
constexpr int8  INT8_MIN_  = -128;
constexpr int16 INT16_MAX_ = 32767;
constexpr int16 INT16_MIN_ = -32768;
constexpr int32 INT32_MAX_ = 0x7fffffff;
constexpr int32 INT32_MIN_ = 0x80000000;
constexpr int64 INT64_MAX_ = 0x7fffffffffffffff;
constexpr int64 INT64_MIN_ = 0x8000000000000000;

constexpr float32 FLT32_MAX = 3.402823466e+38f;
constexpr float32 FLT32_MIN = 1.175494351e-38f;
constexpr float64 FLT64_MAX = 1.7976931348623158e+308;
constexpr float64 FLT64_MIN = 2.2250738585072014e-308;
